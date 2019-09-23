# julius-json-plugin
Julius plugin for output json to stdout

## Quick usage using macOS

```shell
mkdir ~/your_workspace
cd ~/your_workspace

# build julius-json-plugin
git clone git@github.com:kyokuheki/julius-json-plugin.git
cd julius-json-plugin
make
ls -al output_json.jpi

# get latest dictation-kit
cd ~/your_workspace
wget https://osdn.net/dl/julius/dictation-kit-4.5.zip
unzip dictation-kit-4.5.zip
cd dictation-kit-4.5
chmod +x bin/osx/julius

# specify plugindir and add -json option to use julius-json-plugin
./bin/osx/julius -C main.jconf -C am-gmm.jconf -demo -plugindir ~/your_workspace/julius-json-plugin -json
```

## How to build

```shell
mkdir ~/your_workspace
cd ~/your_workspace
git clone git@github.com:kyokuheki/julius-json-plugin.git
cd julius-json-plugin
make
ls -al output_json.jpi
```

## Usage
Specify plugindir and add -json option if you use this plugin. 

```shell
julius -plugindir /path/to/dir/of/output_json.jpi -json ...
```

## Example of output

The plugin outputs JSON data with a prefix `JSON> `.
Since Julius outputs a lot of information to STDOUT, this stupid prefix may be useful when parsing.

```
JSON> {"TIME":{"LISTEN":1569247983,"STARTREC":1569247991,"ENDREC":1569247992},"PASS1":[{"ID":0,"NAME":"_default","STATUS":"SUCCESS","succeeded":true}],"INPUT":{"FRAMES":128,"MSEC":1280},"RECOGOUT":[{"ID":0,"NAME":"_default","STATUS":"SUCCESS","succeeded":true,"SHYPO":[{"RANK":1,"SCORE":-3317.919921875,"WHYPO":[{"WORD":"","CLASSID":"<s>","PHONE":"silB","CM":0.73259258270263672},{"WORD":"こんにちは","CLASSID":"こんにちは+感動詞","PHONE":"k o N n i ch i w a","CM":0.7257799506187439},{"WORD":"。","CLASSID":"<\/s>","PHONE":"silE","CM":1}]}]}],"sentence":"こんにちは 。","succeeded":true,"result":[{"ID":0,"NAME":"_default","STATUS":"SUCCESS","succeeded":true}]}
```

The plugin outputs status messages, like `INPUT STATUS=STARTREC`, `CALLBACK_EVENT_RECOGNITION_ENDSTAT`, and so on.
`INPUT STATUS=STARTREC` means start of a record.
`CALLBACK_EVENT_RECOGNITION_ENDSTAT` means end of a recognition.
Since Julius recognizes speech continuously, these messages are useful when detecting breaks of speech recognitions.

```
<<< please speak >>>
STAT: JSON: INPUT STATUS=STARTREC TIME=1569247991
STAT: JSON: PASS1_STARTRECOG
STAT: JSON: PASS1_INTERIM
pass1_best:STAT: JSON: PASS1_INTERIM
pass1_best:  五STAT: JSON: PASS1_INTERIM
pass1_best:  今STAT: JSON: PASS1_INTERIM
pass1_best:  今日STAT: JSON: INPUT STATUS=ENDREC TIME=1569247992
STAT: JSON: PASS1_INTERIM
pass1_best:  こんにちはsucceeded:true, ID:SR00, NAME:_default, STATUS:SUCCESS
pass1_best:  こんにちは                         
STAT: JSON: PASS1_ENDRECOG
sentence1:  こんにちは 。
JSON> {"TIME":{"LISTEN":1569247983,"STARTREC":1569247991,"ENDREC":1569247992},"PASS1":[{"ID":0,"NAME":"_default","STATUS":"SUCCESS","succeeded":true}],"INPUT":{"FRAMES":128,"MSEC":1280},"RECOGOUT":[{"ID":0,"NAME":"_default","STATUS":"SUCCESS","succeeded":true,"SHYPO":[{"RANK":1,"SCORE":-3317.919921875,"WHYPO":[{"WORD":"","CLASSID":"<s>","PHONE":"silB","CM":0.73259258270263672},{"WORD":"こんにちは","CLASSID":"こんにちは+感動詞","PHONE":"k o N n i ch i w a","CM":0.7257799506187439},{"WORD":"。","CLASSID":"<\/s>","PHONE":"silE","CM":1}]}]}],"sentence":"こんにちは 。","succeeded":true,"result":[{"ID":0,"NAME":"_default","STATUS":"SUCCESS","succeeded":true}]}
STAT: JSON: CALLBACK_EVENT_RECOGNITION_ENDSTAT: JSON: INPUT STATUS=LISTEN TIME=1569247992
<<< please speak >>>
```
