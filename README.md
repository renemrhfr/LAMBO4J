# LAMBO4J (Language Model Bindings - Optimized for Java)
![llama](logo800x800.png)

LAMBO4J makes it possible to locally run LargeLanguageModels (LLMs) like Llama, Mistral and DeepSeek within your Java Application.

It acts as a Wrapper for the amazing [Llama.cpp](https://github.com/ggerganov/llama.cpp) Project and makes it super-easy to write LLM-Driven Java Applications without the need of any 3rd Party tools.

You are in full control of how you use the LanguageModel and don't have to trust a library or a closed-source program how your resources and ports are exposed.

## Project Strucutre
- In the .cpp Folder you find the code for llama.cpp, aswell as a wrapper for simple usage and the JNI Classes for binding to Java. You use this to compile the libraries for your OS/Architecture or further optimize/change the code. You only need to do this once, after that only need the Java Part! I plan to provide pre-compiled Libraries, so in the future you'll be able to skip this part.
- In the Java Folder you will find a usage example in Main.java. I suggest to use this as a starting point and get creative from there!

## Quick Start
1. Download a .GGUF model from Hugging Face and put it in the /models/ Folder
2. Navigate to the /java/ Folder and change the model-name accordingly for instantiating the LanguageModel Instance:
```LanguageModel llama = new LanguageModel("../models/llama3-8b.gguf", 2048, 99)```
3. Open the cpp Folder and run the lambo4j goal
4. Copy the generated library from the release folder to ```java/src/main/resources/{your-os-and-architecture}```
5. Run Main.java and enter your first Prompt via the Command-Line!

## C++ Folder
The cpp-Folder holds the core of [Llama.cpp](https://github.com/ggerganov/llama.cpp), enabling inference and the wrapper + JNI Code.

Inside the lambo4j folder, you can find the things you are most likely to adjust with ```com_renemrhfr_LanguageModel.h``` and ```jni_implementation.cpp``` acting as the bridge to Java  and ```llamawrapper``` being the class that handles 
the communication with the Large Language Model via llama.cpp.

You can build the library with the lambo4j goal in CMake. Put it in the resources folder of the Java Project and load it from there.
Note that NativeLibraryLoader.java checks your os and builds up a folder name from there. you can remove or adjust this for your needs.

## Usage in your Projects
In your Java Code, initialize an Instance of com.renemrhfr.LanguageModel like this:

```
LanguageModel llama3 = new LanguageModel("../models/llama3-8b.gguf", 2048, 99")
```

It was important to me to bring full control of the Conversation-History and System Prompt to the Java World, so you need to manage the conversation history in your Java Code.

You can add messages by calling the .addMessage Method:

```
llama3.addMessage("user", "Is Mayonnaise an instrument?");
```

where the first Parameter can be "user", "assistant" or "system".

As the Method .addMessage returns a TokenCallback - which is a functional Interface with a Method that expects a string -  you are completely flexible in how you process the response token by token as its generated.

If you haven't heard about functional interfaces yet: this means you can implement what should happen inside a lambda expression (or alternatively a method reference):

```
var response = llama.generateResponse(token -> {
                    System.out.print(token);
                    System.out.flush();
                });
```

## ToDo
The Project is still a Work-In-Progress, my next goals are:

- [ ] Docker-Support
- [ ] More comprehensive documentation, especially in the code
- [ ] Enums for Roles
- [ ] Further cleanup unused C++ Code and push a minimal package to this repo
- [ ] Support for Quick Text Embeddings, for example in RAG-Projects.
