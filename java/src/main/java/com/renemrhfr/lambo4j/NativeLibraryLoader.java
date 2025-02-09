package com.renemrhfr.lambo4j;

import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.nio.file.*;
import java.util.Locale;

public class NativeLibraryLoader {
    private static final String LIBRARY_NAME = "lambo4j";
    
    private static String getLibraryPath() {
        String os = System.getProperty("os.name").toLowerCase(Locale.ROOT);
        String arch = System.getProperty("os.arch").toLowerCase(Locale.ROOT);
        
        String prefix = "lib";
        String suffix;
        String dirName;
        
        if (os.contains("windows")) {
            suffix = ".dll";
            dirName = "win-" + getArchName(arch);
        } else if (os.contains("mac") || os.contains("darwin")) {
            suffix = ".dylib";
            dirName = "macos-" + getArchName(arch);
        } else if (os.contains("linux")) {
            suffix = ".so";
            dirName = "linux-" + getArchName(arch);
        } else {
            throw new UnsupportedOperationException("Unsupported operating system: " + os);
        }
        
        return "/native/" + dirName + "/" + prefix + LIBRARY_NAME + suffix;
    }
    
    private static String getArchName(String arch) {
        if (arch.contains("amd64") || arch.contains("x86_64")) {
            return "x86_64";
        } else if (arch.contains("aarch64")) {
            return "arm64";
        } else {
            throw new UnsupportedOperationException("Unsupported architecture: " + arch);
        }
    }
    
    public static void loadLibrary() {
        try {
            // First try loading from Java library path
            System.loadLibrary(LIBRARY_NAME);
        } catch (UnsatisfiedLinkError e) {
            // If that fails, try loading from our bundled resources
            loadBundledLibrary();
        }
    }
    
    private static void loadBundledLibrary() {
        String libraryPath = getLibraryPath();
        
        try (InputStream is = NativeLibraryLoader.class.getResourceAsStream(libraryPath)) {
            if (is == null) {
                throw new RuntimeException("Could not find native library: " + libraryPath);
            }
            
            // Create temporary directory if it doesn't exist
            Path tempDir = Paths.get(System.getProperty("java.io.tmpdir"), "lambo4j");
            Files.createDirectories(tempDir);
            
            // Create temporary file
            Path temp = tempDir.resolve(new File(libraryPath).getName());
            
            // Copy library to temporary file if it doesn't exist or if it's outdated
            if (!Files.exists(temp) || Files.size(temp) != is.available()) {
                Files.copy(is, temp, StandardCopyOption.REPLACE_EXISTING);
            }
            
            // Load the library
            System.load(temp.toAbsolutePath().toString());
            
        } catch (IOException e) {
            throw new RuntimeException("Failed to load native library", e);
        }
    }
}