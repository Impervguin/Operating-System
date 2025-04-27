show:
	-sudo insmod md2.ko

	@sleep 1

	sudo insmod md1.ko
	sudo insmod md2.ko

	@sleep 1

	-sudo rmmod md1.ko

	@sleep 1

	-sudo rmmod md2.ko
	-sudo rmmod md1.ko

	@sleep 1

	-sudo insmod md1.ko
	-sudo insmod md3.ko

	lsmod | grep -E 'md[0-9]'

	@sudo rmmod md1.ko




