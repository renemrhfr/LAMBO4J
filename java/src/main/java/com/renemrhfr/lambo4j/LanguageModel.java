package com.renemrhfr.lambo4j;

import java.util.ArrayList;
import java.util.List;

public class LanguageModel implements AutoCloseable {
    static {
        NativeLibraryLoader.loadLibrary();
    }

    private long nativeHandle;
    private List<ChatMessage> messages = new ArrayList<>();
    private volatile boolean isClosed = false;

    public LanguageModel(String modelPath, int contextSize, int gpuLayers) {
        nativeHandle = initialize(modelPath, contextSize, gpuLayers);
        if (nativeHandle == 0) {
            throw new IllegalStateException("Failed to initialize native llama wrapper");
        }
    }

    public void addMessage(String role, String content) {
        ensureOpen();
        messages.add(new ChatMessage(role, content));
    }

    public String generateResponse(TokenCallback callback) {
        ensureOpen();
        return generateResponseNative(nativeHandle, messages, callback);
    }

    public String generateResponse() {
        return generateResponse(null);
    }

    public void clearHistory() {
        ensureOpen();
        messages.clear();
    }

    private void ensureOpen() {
        if (isClosed) {
            throw new IllegalStateException("LlamaWrapper has been closed");
        }
    }

    private native long initialize(String modelPath, int contextSize, int gpuLayers);
    private native String generateResponseNative(long handle, List<ChatMessage> messages, TokenCallback callback);
    private native void cleanup(long handle);

    @Override
    public void close() {
        if (!isClosed) {
            synchronized (this) {
                if (!isClosed) {
                    if (nativeHandle != 0) {
                        cleanup(nativeHandle);
                        nativeHandle = 0;
                    }
                    isClosed = true;
                }
            }
        }
    }
}