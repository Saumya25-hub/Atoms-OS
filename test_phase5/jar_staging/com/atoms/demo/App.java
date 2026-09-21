package com.atoms.demo;

import java.util.ArrayList;

public class App {
    private String name;
    private ArrayList<String> tasks;

    public App(String appName) {
        this.name = appName;
        this.tasks = new ArrayList<String>();
    }

    public void addTask(String task) {
        this.tasks.add(task);
    }

    public int getTaskCount() {
        return this.tasks.size();
    }

    public String getTask(int index) {
        return this.tasks.get(index);
    }

    public String getName() {
        return this.name;
    }
}
