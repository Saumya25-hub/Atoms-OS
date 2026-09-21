package com.atoms.demo;

public class Utils {
    public static String formatHeader(String title) {
        StringBuilder sb = new StringBuilder();
        sb.append("=== [");
        sb.append(title);
        sb.append("] ===");
        return sb.toString();
    }

    public static int parseCount(String text) {
        return Integer.parseInt(text);
    }
}
