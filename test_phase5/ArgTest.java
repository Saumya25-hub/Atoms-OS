public class ArgTest {
    public static void main(String[] args) {
        if (args != null) {
            System.out.println(args.length);
            for (int i = 0; i < args.length; i++) {
                System.out.println(args[i]);
            }
        } else {
            System.out.println(0);
        }
    }
}
