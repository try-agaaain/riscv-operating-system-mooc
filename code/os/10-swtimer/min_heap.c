#include <stdio.h>
#include <assert.h>

#define MAX_HEAP 100 // 堆的最大容量

// 堆元素的类型
typedef struct {
    int key; // 堆元素的关键字，用于比较
    // 可以添加其他需要的数据
} heap_element;

// 最小堆的结构体
typedef struct {
    heap_element elements[MAX_HEAP]; // 存储堆元素的数组
    int size; // 堆当前的大小
} min_heap;

// 初始化堆
void heap_init(min_heap *h) {
    h->size = 0; // 初始化堆的大小为0
}

// 向下调整堆
void heap_percolate_down(min_heap *h, int hole) {
    int child;
    heap_element tmp = h->elements[hole];

    for (; hole * 2 + 1 < h->size; hole = child) {
        child = hole * 2 + 1; // 左子节点的索引
        // 选择两个子节点中较小的一个
        if (child != h->size - 1 && h->elements[child + 1].key < h->elements[child].key) {
            child++; // 右子节点的索引
        }
        // 如果子节点更小，则向下移动子节点
        if (h->elements[child].key < tmp.key) {
            h->elements[hole] = h->elements[child];
        } else {
            break;
        }
    }
    h->elements[hole] = tmp;
}

// 向上调整堆
void heap_percolate_up(min_heap *h, int hole) {
    heap_element tmp = h->elements[hole];
    int parent = (hole - 1) / 2; // 父节点的索引

    while (hole > 0 && h->elements[parent].key > tmp.key) {
        h->elements[hole] = h->elements[parent]; // 向上移动父节点
        hole = parent;
        parent = (hole - 1) / 2;
    }
    h->elements[hole] = tmp;
}

// 插入元素到堆中
void heap_insert(min_heap *h, heap_element value) {
    if (h->size >= MAX_HEAP) {
        // 堆已满，无法插入
        return;
    }
    // 将新元素添加到堆的末尾
    int hole = h->size++;
    h->elements[hole] = value;
    // 向上调整堆，以保持最小堆的性质
    heap_percolate_up(h, hole);
}

// 删除堆顶元素
heap_element heap_delete_min(min_heap *h) {
    if (h->size <= 0) {
        // 堆为空，返回一个默认值
        heap_element empty = {0};
        return empty;
    }
    // 获取堆顶元素
    heap_element min_element = h->elements[0];
    // 将最后一个元素移动到堆顶
    h->elements[0] = h->elements[--h->size];
    // 向下调整堆
    heap_percolate_down(h, 0);
    // 返回堆顶元素
    return min_element;
}

// 获取堆顶元素（但不删除）
heap_element heap_get_min(min_heap *h) {
    if (h->size <= 0) {
        // 堆为空，返回一个默认值
        heap_element empty = {0};
        return empty;
    }
    // 返回堆顶元素
    return h->elements[0];
}

// 检查堆是否为空
int heap_is_empty(min_heap *h) {
    return h->size == 0;
}



// 测试函数
void test_min_heap() {
    min_heap h;
    heap_init(&h); // 始化堆

    // 测试数据
    int test_data[] = {3, 10, 4, 5, 1, 6};
    int test_data_size = sizeof(test_data) / sizeof(test_data[0]);

    // 插入测试数据到堆中
    for (int i = 0; i < test_data_size; ++i) {
        heap_element elem;
        elem.key = test_data[i];
        heap_insert(&h, elem);
        printf("Inserted %d into the heap.\n", test_data[i]);
    }

    // 检查堆顶元素是否为最小值
    heap_element min_elem = heap_get_min(&h);
    assert(min_elem.key == 1); // 堆顶元素应该是1
    printf("The minimum element is %d.\n", min_elem.key);

    // 删除元素并检查堆的顺序
    printf("Removing elements from the heap:\n");
    while (!heap_is_empty(&h)) {
        min_elem = heap_delete_min(&h);
        printf("%d ", min_elem.key);
        // 检查删除的元素是否是当前最小的
        if (!heap_is_empty(&h)) {
            assert(min_elem.key <= heap_get_min(&h).key);
        }
    }
    printf("\nAll elements removed from the heap.\n");

    // 检查堆是否为空
    assert(heap_is_empty(&h));
    printf("The heap is now empty.\n");
}

int main() {
    test_min_heap(); // 运行测试函数
    return 0;
}