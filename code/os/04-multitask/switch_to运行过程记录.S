# 假设仅有两个任务，上下文分别为context0和context1
# 第一次执行，mscratch初始为0
switch_to:
	csrrw	t6, mscratch, t6    # 交换后t6=0
	beqz	t6, 1f  			# 跳转到标签1
	reg_save t6
	mv	t5, t6
	csrr	t6, mscratch
	sw	t6, 120(t5)
1:
	csrw	mscratch, a0    # 将context0的首地址赋给mscratch
	mv	t6, a0              # 将context0的首地址地址赋给t6
	reg_restore t6          # 将context0的信息恢复到寄存器，其中t6也会被重新恢复
	ret                     # 跳转到ra寄存器保存的地址并执行
.end
# 第二次执行，mscratch为全局变量，它的值保持为上一次赋予的值即context0
switch_to:
	csrrw	t6, mscratch, t6    # 交换后t6=context0，mscratch保存了t6原本的值
	beqz	t6, 1f              # 不跳转，顺序执行
	reg_save t6                 # 保存所有寄存器的值到context0（除t6寄存器）（ra为调用switch_to时的下一条命令）
	mv	t5, t6                  # 将t6的值（context0的首地址）保存到t5
	csrr	t6, mscratch    # 将mscratch的值【即t6原本的值】保存给t6
	sw	t6, 120(t5)         # 将t6保存到context0
1:                          # 继续顺序执行
	csrw	mscratch, a0    # 将context1的首地址赋给mscratch
	mv	t6, a0              # 将context1的首地址地址赋给t6
	reg_restore t6          # 将context1的信息恢复到寄存器，其中t6会被重新恢复
	ret                     # 跳转到ra寄存器保存的地址并执行
.end
# 第三次执行，mscratch为全局变量，它的值保持为上一次赋予的值即context1
switch_to:
	csrrw	t6, mscratch, t6    # 交换后t6=context1，mscratch中暂存t6原本的值
	beqz	t6, 1f              # 不跳转，顺序执行
	reg_save t6                 # 保存所有寄存器的值到context1（除t6寄存器）
	mv	t5, t6                  # 将t6的值（context0的首地址）保存到t5
	csrr	t6, mscratch    # 将mscratch的值【即t6原本的值】保存给t6
	sw	t6, 120(t5)         # 将t6保存到context1
1:                          # 继续顺序执行
	csrw	mscratch, a0    # 将context0的首地址赋给mscratch
	mv	t6, a0              # 将context0的首地址地址赋给t6
	reg_restore t6          # 将context0的信息恢复到寄存器，其中t6会被重新恢复
	ret                     # 跳转到ra寄存器保存的地址并执行
.end  