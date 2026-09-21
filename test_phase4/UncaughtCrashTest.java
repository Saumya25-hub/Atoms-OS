public class UncaughtCrashTest {
    public static void main(String[] args) {
        System.out.println("About to throw uncaught exception");
        throw new NullPointerException("Fatal NPE in ATOMS OS userspace");
    }
}
