typedef void* intr_handler; // intr_handler是个空指针类型，该指针没有具体的类型，仅仅用来表示地址。  这是因为intr_handler是用来修饰intr_entry_table的，intr_entry_table中的元素都是普通地址。
