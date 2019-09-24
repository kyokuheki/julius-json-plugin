/*
 * Copyright (c) 1991-2016 Kawahara Lab., Kyoto University
 * Copyright (c) 2000-2005 Shikano Lab., Nara Institute of Science and Technology
 * Copyright (c) 2005-2016 Julius project team, Nagoya Institute of Technology
 * All rights reserved
 */

#include <julius/juliuslib.h>
#include <stdio.h>
#include "parson.h"

static int outout_json_flag = 0;
static JSON_Value *root_value = NULL;
static JSON_Object *root_object = NULL;

static void
json_init()
{
  if (root_value != NULL) json_value_free(root_value);
  root_value = json_value_init_object();
  root_object = json_value_get_object(root_value);
  return;
}

#ifdef CHARACTER_CONVERSION
#define MAXBUFLEN 4096 ///< Maximum line length of a message sent from a client
static char inbuf[MAXBUFLEN];
static char outbuf[MAXBUFLEN];
static void
output(char *fmt, ...)
{
  va_list ap;
  int ret;
  char *buf;

  va_start(ap,fmt);
  ret = vsnprintf(inbuf, MAXBUFLEN, fmt, ap);
  va_end(ap);
  if (ret > 0) {		/* success */
    printf("%s", charconv(inbuf, outbuf, MAXBUFLEN));
    fflush(stdout);
  }
}
#else
#define output printf
#endif

#define jsonify(KEY,VALUE) output("\"%s\":\"%s\"", KEY, VALUE)
#define jsonify_fmt(KEY,VALUE,FMT) output("\"%s\":\"" FMT "\"", KEY, VALUE)
#define jsonify_integer(KEY,VALUE) output("\"%s\":%d", KEY, VALUE)
#define jsonify_float(KEY,VALUE) output("\"%s\":%1.17g", KEY, VALUE)
#define jsonify_string(KEY,VALUE) output("\"%s\":\"%s\"", KEY, VALUE)
#define jsonify_boolean(KEY,VALUE) do { \
       if (VALUE) output("\"%s\":true", KEY); \
       else output("\"%s\":false", KEY); \
   } while (0)
#define jsonify_sep() output(", ")

static void
nop(void)
{
  fflush(stdout);
}

static boolean
opt_json(Jconf *jconf, char *arg[], int argnum)
{
  outout_json_flag = 1;
  return TRUE;
}

int
initialize()
{
  j_add_option("-json", 0, 0, "enable json extension", opt_json);
  setvbuf(stdout, (char *)NULL, _IONBF, 0);
  json_init();
  return 0;
}

int
get_plugin_info(int opcode, char *buf, int buflen)
{
  switch(opcode) {
  case 0:
    strncpy(buf, "output result with json format", buflen);
    break;
  }
  return 0;
}

void
result_best_str(char *result_str)
{
  if (result_str == NULL) {
    json_object_set_string(root_object, "sentence", "");
  } else {
    json_object_set_string(root_object, "sentence", result_str);
  }
  json_object_set_boolean(root_object, "succeeded", result_str != NULL);
}

/**
 * Subroutine to output information of a recognition status.
 */
static const char *get_status_info(int status)
{
  const char *info = NULL;
  switch(status) {
  case J_RESULT_STATUS_SUCCESS:
    info = "SUCCESS";
    break;
  case J_RESULT_STATUS_REJECT_POWER:
    info = "REJECTED: by power";
    break;
  case J_RESULT_STATUS_TERMINATE:
    info = "input terminated by request";
    break;
  case J_RESULT_STATUS_ONLY_SILENCE:
    info = "REJECTED: result has pause words only";
    break;
  case J_RESULT_STATUS_REJECT_GMM:
    info = "REJECTED: by GMM";
    break;
  case J_RESULT_STATUS_REJECT_SHORT:
    info = "REJECTED: too short input";
    break;
  case J_RESULT_STATUS_REJECT_LONG:
    info = "REJECTED: too long input";
    break;
  case J_RESULT_STATUS_FAIL:
    info = "RECOGFAIL";
    break;
  default:
    info = "UNKNOWN";
  }
  return info;
}

/**
 * Subroutine to output information of a recognized word at 2nd pass.
 */
static void
word_out_old(WORD_ID w, RecogProcess *r, JSON_Object *jo)
{
  int j;
  static char buf[MAX_HMMNAME_LEN];
  static char phone[MAX_HMMNAME_LEN*100];
  WORD_INFO *winfo;

  winfo = r->lm->winfo;
  jsonify("WORD", winfo->woutput[w]);
  json_object_set_string(jo, "WORD", winfo->woutput[w]);

  jsonify_sep();
  jsonify("CLASSID", winfo->wname[w]);
  json_object_set_string(jo, "CLASSID", winfo->wname[w]);

  for(j=0;j<winfo->wlen[w];j++) {
    center_name(winfo->wseq[w][j]->name, buf);
    if (j == 0) snprintf(phone, MAX_HMMNAME_LEN*100, "%s", buf);
    else snprintf(phone, MAX_HMMNAME_LEN*100, "%s %s", phone, buf);
  }
  jsonify_sep();
  jsonify("PHONE", phone);
  json_object_set_string(jo, "PHONE", phone);
}

/**
 * Subroutine to output information of a recognized word at 2nd pass.
 */
static void
word_out(WORD_ID w, RecogProcess *r, JSON_Object *jo)
{
  int j;
  static char buf[MAX_HMMNAME_LEN];
  static char phone[MAX_HMMNAME_LEN*100];
  WORD_INFO *winfo;
  winfo = r->lm->winfo;

  json_object_set_string(jo, "WORD", winfo->woutput[w]);
  json_object_set_string(jo, "CLASSID", winfo->wname[w]);
  for(j=0;j<winfo->wlen[w];j++) {
    center_name(winfo->wseq[w][j]->name, buf);
    if (j == 0) snprintf(phone, MAX_HMMNAME_LEN*100, "%s", buf);
    else snprintf(phone, MAX_HMMNAME_LEN*100, "%s %s", phone, buf);
  }
  json_object_set_string(jo, "PHONE", phone);
}

/**
 * 1st pass: output when recognition begins (will be called at input start).
 */
static void
status_pass1_begin(Recog *recog, void *dummy)
{
  jlog("STAT: JSON: PASS1_STARTRECOG\n");
}

static void
result_pass1_current(Recog *recog, void *dummy)
{
  jlog("STAT: JSON: PASS1_INTERIM\n");
}

static void
result_pass1_final(Recog *recog, void *dummy)
{
  RecogProcess *r;
  JSON_Value *pass1_array = json_value_init_array();
  json_object_set_value(root_object, "PASS1", pass1_array);

  for(r=recog->process_list;r;r=r->next) {
    if (! r->live) continue;
    JSON_Value *pass1_value = json_value_init_object();
    json_array_append_value(json_value_get_array(pass1_array), pass1_value);
    JSON_Object *pass1_object = json_value_get_object(pass1_value);

    json_object_set_number(pass1_object, "ID", r->config->id);
    json_object_set_string(pass1_object, "NAME", r->config->name);
    json_object_set_string(pass1_object, "STATUS", get_status_info(r->result.status));
    json_object_set_boolean(pass1_object, "succeeded", r->result.status >= 0);

    if (r->result.status < 0) {
      output("succeeded:false, ID:SR%02d, NAME:%s, STATUS:%s\n", r->config->id, r->config->name, get_status_info(r->result.status));
    } else {
      output("succeeded:true, ID:SR%02d, NAME:%s, STATUS:%s\n", r->config->id, r->config->name, get_status_info(r->result.status));
    }
  }
}

/**
 * 1st pass: end of output (will be called at the end of the 1st pass).
 */
static void
status_pass1_end(Recog *recog, void *dummy)
{
  jlog("STAT: JSON: PASS1_ENDRECOG\n");
  nop();
}

/**
 * 2nd pass: output a sentence hypothesis found in the 2nd pass.
 *
 * @param hypo [in] sentence hypothesis to be output
 * @param rank [in] rank of @a hypo
 * @param winfo [in] word dictionary
 */
static void
result_pass2_old(Recog *recog, void *dummy)
{
  int i, n, num;
  WORD_INFO *winfo;
  WORD_ID *seq;
  int seqnum;
  Sentence *s;
  RecogProcess *r;
  SentenceAlign *align;

  //JSON_Value *root_value = json_value_init_object();
  //JSON_Object *root_object = json_value_get_object(root_value);

  JSON_Value *json_recog = json_value_init_array();
  json_object_set_value(root_object, "RECOGOUT", json_recog);
  output("{\"RECOGOUT\":[\n");
  for(r=recog->process_list;r;r=r->next) {
    if (! r->live) continue;
    JSON_Value *recogout_value = json_value_init_object();
    json_array_append_value(json_value_get_array(json_recog), recogout_value);
    JSON_Object *recogout_object = json_value_get_object(recogout_value);

    output("  {");

    jsonify_fmt("ID", r->config->id, "SR%02d");
    json_object_set_number(recogout_object, "ID", r->config->id);

    jsonify_sep();
    jsonify("NAME", r->config->name);
    json_object_set_string(recogout_object, "NAME", r->config->name);

    jsonify_sep();
    jsonify("STATUS", get_status_info(r->result.status));
    json_object_set_string(recogout_object, "STATUS", get_status_info(r->result.status));

    if (r->result.status < 0) {
      json_object_set_boolean(recogout_object, "succeeded", FALSE);
      jsonify_sep();
      jsonify_boolean("succeeded", FALSE);
      output("}\n");
      printf("JSON> %s\n", json_serialize_to_string(root_value));
      continue;
    } else {
      json_object_set_boolean(recogout_object, "succeeded", TRUE);
      jsonify_sep();
      jsonify_boolean("succeeded", TRUE);
    }

    winfo = r->lm->winfo;
    //num = r->result.sentnum;

    jsonify_sep();
    output("\n    \"SHYPO\":[\n");
    JSON_Value *json_shypo = json_value_init_array();
    json_object_dotset_value(recogout_object, "SHYPO", json_shypo);

    for(n=0;n<r->result.sentnum;n++) {
      JSON_Value *shypo_value = json_value_init_object();
      json_array_append_value(json_value_get_array(json_shypo), shypo_value);
      JSON_Object *shypo_object = json_value_get_object(shypo_value);

      s = &(r->result.sent[n]);
      seq = s->word;
      seqnum = s->word_num;

      output("      {");
      jsonify_integer("RANK", n+1);
      json_object_set_number(shypo_object, "RANK", n+1);
#ifdef USE_MBR
      if(r->config->mbr.use_mbr == TRUE){
        jsonify_sep();
        jsonify_float("MBRSCORE", s->score_mbr);
        json_object_set_number(shypo_object, "MBRSCORE", s->score_mbr);
      }
#endif
      jsonify_sep();
      jsonify_float("SCORE", s->score);
      json_object_set_number(shypo_object, "SCORE", s->score);
      if (r->lmtype == LM_PROB) {
        jsonify_sep();
        jsonify_float("AMSCORE", s->score_am);
        json_object_set_number(shypo_object, "AMSCORE", s->score_am);
        jsonify_sep();
        jsonify_float("LMSCORE", s->score_lm);
        json_object_set_number(shypo_object, "LMSCORE", s->score_lm);
      }
      if (r->lmtype == LM_DFA) {
        /* output which grammar the best hypothesis belongs to */
        jsonify_sep();
        jsonify_integer("GRAM", s->gram_id);
        json_object_set_number(shypo_object, "GRAM", s->gram_id);
      }

      /*** WHYPO ***/
      jsonify_sep();
      output("\n        \"WHYPO\":[\n");
      JSON_Value *json_whypo = json_value_init_array();
      json_object_set_value(shypo_object, "WHYPO", json_whypo);

      for (i=0;i<seqnum;i++) {
        JSON_Value *whypo_value = json_value_init_object();
        json_array_append_value(json_value_get_array(json_whypo), whypo_value);
        JSON_Object *whypo_object = json_value_get_object(whypo_value);

        output("          {");
        word_out(seq[i], r, whypo_object);
#ifdef CONFIDENCE_MEASURE
#ifdef CM_MULTIPLE_ALPHA
        /* currently not handle multiple alpha output */
#else
        jsonify_sep();
        jsonify_float("CM", s->confidence[i]);
        json_object_set_number(whypo_object, "CM", s->confidence[i]);
#endif
#endif /* CONFIDENCE_MEASURE */
        /* output alignment result if exist */
        for (align = s->align; align; align = align->next) {
          switch(align->unittype) {
            case PER_WORD:	/* word alignment */
              jsonify_sep();
              jsonify_integer("BEGINFRAME", align->begin_frame[i]);
              json_object_set_number(whypo_object, "BEGINFRAME", align->begin_frame[i]);
              jsonify_sep();
              jsonify_integer("ENDFRAME", align->end_frame[i]);
              json_object_set_number(whypo_object, "ENDFRAME", align->end_frame[i]);
              break;
            case PER_PHONEME:
            case PER_STATE:
              fprintf(stderr, "Error: \"-palign\" and \"-salign\" does not supported for module output\n");
              break;
          }
        }
        if (i+1<seqnum) output("},\n");
        else output("}\n");
      }
      output("        ]\n");
      if (n+1<r->result.sentnum) output("      },\n");
      else output("      }\n");
    }
    output("    ]\n");

    if (r->next) output("  },\n");
    else output("  }]\n");
  }
  output("}\n");
  return;
}

/**
 * 2nd pass: output a sentence hypothesis found in the 2nd pass.
 *
 * @param hypo [in] sentence hypothesis to be output
 * @param rank [in] rank of @a hypo
 * @param winfo [in] word dictionary
 */
static void
result_pass2(Recog *recog, void *dummy)
{
  int i, n, num;
  WORD_INFO *winfo;
  WORD_ID *seq;
  int seqnum;
  Sentence *s;
  RecogProcess *r;
  SentenceAlign *align;

  JSON_Value *json_recog = json_value_init_array();
  json_object_set_value(root_object, "RECOGOUT", json_recog);
  for(r=recog->process_list;r;r=r->next) {
    if (! r->live) continue;
    JSON_Value *recogout_value = json_value_init_object();
    json_array_append_value(json_value_get_array(json_recog), recogout_value);
    JSON_Object *recogout_object = json_value_get_object(recogout_value);

    json_object_set_number(recogout_object, "ID", r->config->id);
    json_object_set_string(recogout_object, "NAME", r->config->name);
    json_object_set_string(recogout_object, "STATUS", get_status_info(r->result.status));
    json_object_set_boolean(recogout_object, "succeeded", r->result.status >= 0);

    if (r->result.status < 0) {
      continue;
    }
    winfo = r->lm->winfo;
    JSON_Value *json_shypo = json_value_init_array();
    json_object_dotset_value(recogout_object, "SHYPO", json_shypo);

    for(n=0;n<r->result.sentnum;n++) {
      JSON_Value *shypo_value = json_value_init_object();
      json_array_append_value(json_value_get_array(json_shypo), shypo_value);
      JSON_Object *shypo_object = json_value_get_object(shypo_value);

      s = &(r->result.sent[n]);
      seq = s->word;
      seqnum = s->word_num;

      json_object_set_number(shypo_object, "RANK", n+1);
#ifdef USE_MBR
      if(r->config->mbr.use_mbr == TRUE){
        json_object_set_number(shypo_object, "MBRSCORE", s->score_mbr);
      }
#endif
      json_object_set_number(shypo_object, "SCORE", s->score);
      if (r->lmtype == LM_PROB) {
        json_object_set_number(shypo_object, "AMSCORE", s->score_am);
        json_object_set_number(shypo_object, "LMSCORE", s->score_lm);
      }
      if (r->lmtype == LM_DFA) {
        /* output which grammar the best hypothesis belongs to */
        json_object_set_number(shypo_object, "GRAM", s->gram_id);
      }

      /*** WHYPO ***/
      JSON_Value *json_whypo = json_value_init_array();
      json_object_set_value(shypo_object, "WHYPO", json_whypo);

      for (i=0;i<seqnum;i++) {
        JSON_Value *whypo_value = json_value_init_object();
        json_array_append_value(json_value_get_array(json_whypo), whypo_value);
        JSON_Object *whypo_object = json_value_get_object(whypo_value);

        word_out(seq[i], r, whypo_object);
#ifdef CONFIDENCE_MEASURE
#ifdef CM_MULTIPLE_ALPHA
        /* currently not handle multiple alpha output */
#else
        json_object_set_number(whypo_object, "CM", s->confidence[i]);
#endif
#endif /* CONFIDENCE_MEASURE */
        /* output alignment result if exist */
        for (align = s->align; align; align = align->next) {
          switch(align->unittype) {
            case PER_WORD:	/* word alignment */
              json_object_set_number(whypo_object, "BEGINFRAME", align->begin_frame[i]);
              json_object_set_number(whypo_object, "ENDFRAME", align->end_frame[i]);
              break;
            case PER_PHONEME:
            case PER_STATE:
              fprintf(stderr, "Error: \"-palign\" and \"-salign\" does not supported for module output\n");
              break;
          }
        }
      }
    }
  }
  return;
}


/**
 * Output when ready to recognize and start waiting speech input.
 */
static void
status_recready(Recog *recog, void *dummy)
{
  jlog("STAT: JSON: INPUT STATUS=LISTEN TIME=%ld\n", time(NULL));
  json_object_dotset_number(root_object, "TIME.LISTEN", time(NULL));
  nop();
}

/**
 * Output when input starts.
 */
static void
status_recstart(Recog *recog, void *dummy)
{
  jlog("STAT: JSON: INPUT STATUS=STARTREC TIME=%ld\n", time(NULL));
  json_object_dotset_number(root_object, "TIME.STARTREC", time(NULL));
}
/**
 * Output when input ends.
 */
static void
status_recend(Recog *recog, void *dummy)
{
  jlog("STAT: JSON: INPUT STATUS=ENDREC TIME=%ld\n", time(NULL));
  json_object_dotset_number(root_object, "TIME.ENDREC", time(NULL));
}
/**
 * Output input parameter status such as length.
 */
static void
status_param(Recog *recog, void *dummy)
{
  MFCCCalc *mfcc;
  int frames;
  int msec;

  if (recog->mfcclist->next != NULL)
  {
    JSON_Value *inputstatus = json_value_init_array();
    json_object_set_value(root_object, "INPUT", inputstatus);
    for(mfcc=recog->mfcclist;mfcc;mfcc=mfcc->next) {
      JSON_Value *input = json_value_init_object();
      json_array_append_value(json_value_get_array(inputstatus), input);
      JSON_Object *input_object = json_value_get_object(input);
      frames = mfcc->param->samplenum;
      msec = (float)mfcc->param->samplenum * (float)recog->jconf->input.period * (float)recog->jconf->input.frameshift / 10000.0;
      json_object_dotset_number(input_object, "MFCCID", mfcc->id);
      json_object_dotset_number(input_object, "FRAMES", frames);
      json_object_dotset_number(input_object, "MSEC", msec);
    }
  } else {
    frames = recog->mfcclist->param->samplenum;
    msec = (float)recog->mfcclist->param->samplenum * (float)recog->jconf->input.period * (float)recog->jconf->input.frameshift / 10000.0;
    json_object_dotset_number(root_object, "INPUT.FRAMES", frames);
    json_object_dotset_number(root_object, "INPUT.MSEC", msec);
  }
}

/**
 * Send the result of GMM computation to module client.
 * (for "-result msock" option)
 * </EN>
 */
static void
result_gmm(Recog *recog, void *dummy)
{
  json_object_dotset_string(root_object, "GMM.RESULT", recog->gc->max_d->name);
#ifdef CONFIDENCE_MEASURE
  json_object_dotset_number(root_object, "GMM.CMSCORE", recog->gc->gmm_max_cm);
#endif
}

static void
notify_segment_end(Recog *recog, void *dummy)
{
  jlog("STAT: JSON: SEGMENT_END\n");
}

static void
set_json_result_status(Recog *recog) {
  RecogProcess *r;
  JSON_Value *status_array = json_value_init_array();
  json_object_set_value(root_object, "result", status_array);
  for(r=recog->process_list;r;r=r->next) {
    if (! r->live) continue;
    JSON_Value *status = json_value_init_object();
    json_array_append_value(json_value_get_array(status_array), status);
    JSON_Object *status_object = json_value_get_object(status);

    json_object_set_number(status_object, "ID", r->config->id);
    json_object_set_string(status_object, "NAME", r->config->name);
    json_object_set_string(status_object, "STATUS", get_status_info(r->result.status));
    json_object_set_boolean(status_object, "succeeded", r->result.status >= 0);
  }
}

static void
notify_recog_end(Recog *recog, void *dummy)
{
  set_json_result_status(recog);
  //printf("%s\n", json_serialize_to_string_pretty(root_value));
  printf("JSON> %s\n", json_serialize_to_string(root_value));
  json_init();
  jlog("STAT: JSON: CALLBACK_EVENT_RECOGNITION_END");
}


/**
 * Register output functions to enable json output.
 */
void
setup_output_json(Recog *recog, void *data)
{
  //callback_add(recog, CALLBACK_EVENT_PROCESS_ONLINE, status_process_online, data);
  //callback_add(recog, CALLBACK_EVENT_PROCESS_OFFLINE, status_process_offline, data);
  //callback_add(recog, CALLBACK_EVENT_STREAM_BEGIN,     , data);
  //callback_add(recog, CALLBACK_EVENT_STREAM_END,        , data);
  callback_add(recog, CALLBACK_EVENT_SPEECH_READY, status_recready, data);
  callback_add(recog, CALLBACK_EVENT_SPEECH_START, status_recstart, data);
  callback_add(recog, CALLBACK_EVENT_SPEECH_STOP, status_recend, data);
  callback_add(recog, CALLBACK_EVENT_PASS1_BEGIN, status_pass1_begin, data);
  callback_add(recog, CALLBACK_EVENT_PASS1_END, status_pass1_end, data);
  callback_add(recog, CALLBACK_RESULT_PASS1_INTERIM, result_pass1_current, data);
  callback_add(recog, CALLBACK_RESULT_PASS1, result_pass1_final, data);
  callback_add(recog, CALLBACK_STATUS_PARAM, status_param, data);

  callback_add(recog, CALLBACK_RESULT, result_pass2, data); // rejected, failed
  callback_add(recog, CALLBACK_RESULT_GMM, result_gmm, data);
  /* below will not be called if "-graphout" not specified */
  //callback_add(recog, CALLBACK_RESULT_GRAPH, result_graph, data);

  //callback_add(recog, CALLBACK_EVENT_PAUSE, status_pause, data);
  //callback_add(recog, CALLBACK_EVENT_RESUME, status_resume, data);

  callback_add(recog, CALLBACK_EVENT_SEGMENT_END, notify_segment_end, data);
  callback_add(recog, CALLBACK_EVENT_RECOGNITION_END, notify_recog_end, data);
}

int
startup(void *data)
{
  Recog *recog = data;
  if (outout_json_flag == 1) {
    setup_output_json(recog, data);
  }
  return 0;
}
