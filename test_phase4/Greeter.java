public class Greeter {
    static {
        System.out.println("Greeter: Static initializer <clinit> executed");
    }

    public static void greet() {
        System.out.println("Greeter: Hello from Greeter helper class!");
    }
}
