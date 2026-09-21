package com.atoms.demo;

import java.util.ArrayList;

public class Main {
    public static void main(String[] args) {
        System.out.println(Utils.formatHeader(Config.APP_NAME));
        System.out.println(System.getProperty("os.name"));
        System.out.println(System.getProperty("os.arch"));

        App app = new App("ATOMS Multi-Class Task Manager");
        app.addTask("Initialize Core Subsystems");
        app.addTask("Mount BOFS Storage");
        app.addTask("Execute Native Java Bytecode");

        System.out.println(app.getName());
        System.out.println(app.getTaskCount());

        for (int i = 0; i < app.getTaskCount(); i++) {
            System.out.println(app.getTask(i));
        }

        // Test arguments
        if (args != null && args.length > 0) {
            System.out.println(args.length);
            for (int i = 0; i < args.length; i++) {
                System.out.println(args[i]);
            }
        }
    }
}
