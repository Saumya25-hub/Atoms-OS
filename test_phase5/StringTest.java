public class StringTest {
    public static void main(String[] args) {
        String s = "ATOMS Java Runtime";

        System.out.println(s.length());
        System.out.println(s.startsWith("ATOMS"));
        System.out.println(s.indexOf("Java"));
        System.out.println(s.charAt(0));
        System.out.println(s.substring(0, 5));
    }
}
