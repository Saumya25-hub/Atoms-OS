public class MemoryPressureTest {
    public static void main(String[] args) {
        for (int i = 0; i < 10000; i++) {
            StringBuilder s = new StringBuilder();
            s.append("ATOMS");
            s.append(i);
        }
        System.out.println("Memory pressure test complete");
    }
}
