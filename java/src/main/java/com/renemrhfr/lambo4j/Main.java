package com.renemrhfr.lambo4j;

import java.util.Scanner;

public class Main {

    public static void main(String[] args) {
        var sc = new Scanner(System.in);
        var input = "";
        try (LanguageModel llama = new LanguageModel("../models/llama3-8b.gguf", 2048, 99)) {
            llama.addMessage("system", "You are a helpful AI assistant.");
            System.out.println("Successfully loaded Model. Ready for your prompt!");
            while(input != "q") {
                System.out.println("");
                input = sc.nextLine();
                llama.addMessage("user", input);
                var response = llama.generateResponse(token -> {
                    System.out.print(token);
                    System.out.flush();
                });
                llama.addMessage("assistant", response);
            }
        }
    }

}
