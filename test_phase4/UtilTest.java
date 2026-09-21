public class UtilTest {
    public static void main(String[] args) {
        StringBuilder sb = new StringBuilder();
        sb.append("ATOMS");
        sb.append(" JVM ");
        sb.append("StringBuilder");
        System.out.println(sb.toString());

        java.util.ArrayList list = new java.util.ArrayList();
        list.add("FirstItem");
        list.add("SecondItem");
        System.out.println(list.size());
        System.out.println((String)list.get(0));
        System.out.println((String)list.get(1));
    }
}
