public class ExceptionTest {
    public static void main(String[] args) {
        try {
            System.out.println("Entering try block");
            throwException();
            System.out.println("Unreachable code in try");
        } catch (RuntimeException e) {
            System.out.println("Caught expected RuntimeException!");
        }
    }

    public static void throwException() {
        throw new RuntimeException("ATOMS Exception Simulation");
    }
}
