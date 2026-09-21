public class CollectionsSafetyTest {
    public static void main(String[] args) {
        java.util.ArrayList<String> list = new java.util.ArrayList<String>();
        list.add("BOS");
        list.add("ATOMS");
        list.add("JAVA");

        System.out.println(list.size());
        System.out.println(list.get(1));
        list.set(1, "ATOMS OS");
        System.out.println(list.get(1));
        list.remove(0);
        System.out.println(list.size());
        System.out.println(list.get(0));

        try {
            System.out.println("Probing negative index");
            list.get(-1);
            System.out.println("Unreachable");
        } catch (IndexOutOfBoundsException e) {
            System.out.println("Caught IndexOutOfBoundsException cleanly");
        }
    }
}
