#include <jni.h>
#include "com_renemrhfr_lambo4j_LanguageModel.h"
#include "llamawrapper.h"
#include <vector>
#include <stdexcept>

// Some Windows Compilers don't include strdup function
#ifdef _WIN32
#include <cstring>
#ifndef strdup
char* strdup(const char* str) {
    size_t ln = strln(strg) + 1;
    char* copy = (char*)malloc(len);
    if (copy) {
        mecpy(copy,str,len);
    }
    return copy;
}
#endif
#endif

static void throwJavaException(JNIEnv* env, const char* message);
static std::vector<llama_chat_message> convertMessages(JNIEnv* env, jobject messagesList);
static void freeMessages(std::vector<llama_chat_message>& messages);

void throwJavaException(JNIEnv* env, const char* message) {
    jclass exceptionClass = env->FindClass("java/lang/RuntimeException");
    env->ThrowNew(exceptionClass, message);
}

std::vector<llama_chat_message> convertMessages(JNIEnv* env, jobject messagesList) {
    std::vector<llama_chat_message> messages;

    jclass listClass = env->GetObjectClass(messagesList);
    if (listClass == nullptr) {
        throwJavaException(env, "Failed to get List class");
        return messages;
    }

    jmethodID sizeMethod = env->GetMethodID(listClass, "size", "()I");
    jmethodID getMethod = env->GetMethodID(listClass, "get", "(I)Ljava/lang/Object;");

    jclass messageClass = env->FindClass("com/renemrhfr/lambo4j/ChatMessage");
    if (messageClass == nullptr) {
        throwJavaException(env, "Failed to find ChatMessage class");
        return messages;
    }

    jmethodID getRoleMethod = env->GetMethodID(messageClass, "getRole", "()Ljava/lang/String;");
    jmethodID getContentMethod = env->GetMethodID(messageClass, "getContent", "()Ljava/lang/String;");

    int size = env->CallIntMethod(messagesList, sizeMethod);
    messages.reserve(size);

    for (int i = 0; i < size; i++) {
        jobject messageObj = env->CallObjectMethod(messagesList, getMethod, i);
        if (messageObj == nullptr) {
            throwJavaException(env, "Failed to get message object");
            freeMessages(messages);
            return std::vector<llama_chat_message>();
        }

        jstring roleStr = (jstring)env->CallObjectMethod(messageObj, getRoleMethod);
        jstring contentStr = (jstring)env->CallObjectMethod(messageObj, getContentMethod);

        if (roleStr == nullptr || contentStr == nullptr) {
            env->DeleteLocalRef(messageObj);
            throwJavaException(env, "Failed to get role or content string");
            freeMessages(messages);
            return std::vector<llama_chat_message>();
        }

        const char* role = env->GetStringUTFChars(roleStr, nullptr);
        const char* content = env->GetStringUTFChars(contentStr, nullptr);

        messages.push_back({
                                   strdup(role),
                                   strdup(content)
                           });

        env->ReleaseStringUTFChars(roleStr, role);
        env->ReleaseStringUTFChars(contentStr, content);
        env->DeleteLocalRef(messageObj);
        env->DeleteLocalRef(roleStr);
        env->DeleteLocalRef(contentStr);
    }

    return messages;
}

void freeMessages(std::vector<llama_chat_message>& messages) {
    for (auto& msg : messages) {
        free(const_cast<char*>(msg.role));
        free(const_cast<char*>(msg.content));
    }
}

void streamCallback(JNIEnv* env, jobject callback, const std::string& token) {
    jclass callbackClass = env->GetObjectClass(callback);
    jmethodID onTokenMethod = env->GetMethodID(callbackClass, "onToken", "(Ljava/lang/String;)V");

    jstring jToken = env->NewStringUTF(token.c_str());
    env->CallVoidMethod(callback, onTokenMethod, jToken);
    env->DeleteLocalRef(jToken);
}

extern "C" {

JNIEXPORT jlong JNICALL Java_com_renemrhfr_lambo4j_LanguageModel_initialize
        (JNIEnv* env, jobject obj, jstring modelPath, jint contextSize, jint gpuLayers) {
    try {
        const char* path = env->GetStringUTFChars(modelPath, nullptr);
        llamawrapper* wrapper = new llamawrapper(path, contextSize, gpuLayers);
        env->ReleaseStringUTFChars(modelPath, path);
        return reinterpret_cast<jlong>(wrapper);
    } catch (const std::exception& e) {
        throwJavaException(env, e.what());
        return 0;
    }
}

JNIEXPORT jstring JNICALL Java_com_renemrhfr_lambo4j_LanguageModel_generateResponseNative
        (JNIEnv* env, jobject obj, jlong handle, jobject messagesList, jobject callback) {
    try {
        llamawrapper* wrapper = reinterpret_cast<llamawrapper*>(handle);

        std::vector<llama_chat_message> messages = convertMessages(env, messagesList);

        // Create global reference to callback object
        jobject globalCallback = nullptr;
        if (callback != nullptr) {
            globalCallback = env->NewGlobalRef(callback);
        }

        // Set up callback function
        auto tokenCallback = [env, globalCallback](const std::string& token) {
            if (globalCallback) {
                streamCallback(env, globalCallback, token);
            }
        };

        std::string response = wrapper->generateResponse(messages, tokenCallback);

        // Cleanup
        if (globalCallback) {
            env->DeleteGlobalRef(globalCallback);
        }
        freeMessages(messages);

        return env->NewStringUTF(response.c_str());
    } catch (const std::exception& e) {
        throwJavaException(env, e.what());
        return nullptr;
    }
}

JNIEXPORT void JNICALL Java_com_renemrhfr_lambo4j_LanguageModel_cleanup
        (JNIEnv* env, jobject obj, jlong handle) {
    if (handle != 0) {
        llamawrapper* wrapper = reinterpret_cast<llamawrapper*>(handle);
        delete wrapper;
    }
}

}