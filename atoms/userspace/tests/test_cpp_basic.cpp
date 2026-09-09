/*
 * ATOMS OS — Userspace C++ Runtime Basic Validation Test
 * Tests LLVM libc++ / libc++abi integration:
 * - operator new / delete
 * - polymorphic classes with virtual functions & destructors
 * - static class initialization & construction
 * - memory safety and cleanup
 */

#include <stddef.h>
#include <stdint.h>

extern "C" {
    int snprintf(char *str, size_t size, const char *format, ...);
    size_t strlen(const char *s);
    int64_t write(int fd, const void *buf, size_t count);
}

static void print_msg(const char *msg) {
    write(1, msg, strlen(msg));
}

/* Global static initialization tracker */
static int g_constructor_check = 0;

class GlobalLifecycleTracker {
public:
    GlobalLifecycleTracker() {
        g_constructor_check = 0x1234ABCD;
    }
    ~GlobalLifecycleTracker() {
        g_constructor_check = 0;
    }
};

static GlobalLifecycleTracker s_tracker;

/* Base Polymorphic Class */
class Shape {
public:
    virtual ~Shape() = default;
    virtual int area() const = 0;
    virtual const char *name() const = 0;
};

/* Derived Rectangle */
class Rectangle : public Shape {
private:
    int m_width;
    int m_height;
public:
    Rectangle(int w, int h) : m_width(w), m_height(h) {}
    ~Rectangle() override = default;

    int area() const override {
        return m_width * m_height;
    }
    const char *name() const override {
        return "Rectangle";
    }
};

/* Derived Circle */
class Circle : public Shape {
private:
    int m_radius;
public:
    Circle(int r) : m_radius(r) {}
    ~Circle() override = default;

    int area() const override {
        return (314 * m_radius * m_radius) / 100;
    }
    const char *name() const override {
        return "Circle";
    }
};

extern "C" int main(int argc, char **argv, char **envp) {
    (void)argc; (void)argv; (void)envp;
    char buffer[256];

    print_msg("[ATOMS TEST 2] Starting C++ Runtime Validation Test...\n");

    /* 1. Verify global constructor execution via .init_array */
    if (g_constructor_check != 0x1234ABCD) {
        print_msg("[FATAL] Global static constructor (.init_array) was not executed!\n");
        return 1;
    }
    print_msg("[ATOMS TEST 2] Global constructor execution: PASS\n");

    /* 2. Test LLVM libc++abi operator new and polymorphism */
    Shape *shapes[2];
    shapes[0] = new Rectangle(10, 20);
    shapes[1] = new Circle(5);

    if (!shapes[0] || !shapes[1]) {
        print_msg("[FATAL] operator new returned NULL!\n");
        return 2;
    }

    /* Verify virtual method dispatch */
    int rect_area = shapes[0]->area();
    int circle_area = shapes[1]->area();

    int len = snprintf(buffer, sizeof(buffer),
        "[ATOMS TEST 2] Virtual dispatch: %s area=%d, %s area=%d\n",
        shapes[0]->name(), rect_area, shapes[1]->name(), circle_area);
    write(1, buffer, len);

    if (rect_area != 200) {
        print_msg("[FATAL] Rectangle area incorrect!\n");
        return 3;
    }
    if (circle_area != 78) {
        print_msg("[FATAL] Circle area incorrect!\n");
        return 4;
    }
    print_msg("[ATOMS TEST 2] Polymorphism and virtual dispatch: PASS\n");

    /* 3. Test operator delete */
    delete shapes[0];
    delete shapes[1];
    print_msg("[ATOMS TEST 2] operator delete: PASS\n");

    /* 4. Test dynamic array allocation */
    int *array = new int[128];
    if (!array) {
        print_msg("[FATAL] operator new[] returned NULL!\n");
        return 5;
    }
    for (int i = 0; i < 128; i++) {
        array[i] = i * i;
    }
    for (int i = 0; i < 128; i++) {
        if (array[i] != i * i) {
            print_msg("[FATAL] Array data verification failed!\n");
            return 6;
        }
    }
    delete[] array;
    print_msg("[ATOMS TEST 2] operator new[] and delete[]: PASS\n");

    print_msg("[ATOMS TEST 2] *** ALL C++ RUNTIME TESTS PASSED ***\n");
    return 0;
}
