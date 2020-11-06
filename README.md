# Pof exampleStandalone multithreaded

This example app demonstrates the new multiple-instances possibilities of Pd/libpd.

- a basic "audio.pd" patch runs as usual with ofxPd.
- a basic "gui.pd" patch runs on a secondary thread, and builds a GUI interface using Pof.


The gui patch controls the frequency and the volume of the oscillator contained in the audio patch.

The GUI also proposes to "hang" the gui thread, by asking the gui Pd instance to compute the value of log(2) 10 million times;
this would normally freeze the audio stream during about a second: 
you can test the normal one-thread behaviour by opening "0EditMaster.pd" with Pd.


## Building

This example should build as any other OF app.

### Linux

To build and run it on the terminal:

```
make
make run
```

Optionally, you can build the example with Codeblocks : open the exampleStandalone.cbp and hit F9 to build.

### OSX

Open the XCode project and run.

### Windows

You should be able to generate the project files with the help of OF's projectGenerator.


## application usage : 
```
 exampleStandalone {-inchannels IN} {-outchannels OUT} {-hidecursor}
```
 
