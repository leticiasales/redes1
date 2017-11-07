# https://pymotw.com/2/socket/udp.html

import socket
import sys

def server():
	# Create a TCP/IP socket
	sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

	# Bind the socket to the port
	server_address = ('localhost', 4847)
	print ('starting up on %s port %s' % server_address)
	sock.bind(server_address)

	while True:
	    print ('\nwaiting to receive message')
	    data, address = sock.recvfrom(4096)
	    
	    print ('received %s bytes from %s' % (len(data), address))
	    print data
	    
	    if data:
	        sent = sock.sendto(data, address)
	        print ('sent %s bytes back to %s' % (sent, address))

def client():
	# Create a UDP clisocket
	clisock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

	cli_address = ('localhost', 4847)
	message = 'This is the message.  It will be repeated.'

	try:

	    # Send data
	    print ('sending "%s"' % message)
	    sent = clisock.sendto(message, cli_address)

	    # Receive response
	    print ('waiting to receive')
	    data, server = clisock.recvfrom(4096)
	    print ('received "%s"' % data)

	finally:
	    print ('closing clisocket')
	    clisock.close()

if sys.argv[1] == '0':
	client()
else: 
	server()