
Instructions to use properly the CAN bootloader python script

This is a python script used to send an Arduino sketch by using the CAN bus. 
It will transfer the data through USB to a node in charge of the programming aspect. 
This node will transform USB data into CAN messages and send them to the target.

Open a terminal on the directory of the pyton script.
Simply type :
python bootloader.py -i BOARD_ID -p COM_PORT -f FILE.hex



