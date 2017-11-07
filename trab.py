# https://pymotw.com/2/socket/udp.html

import socket
import sys

# Create a TCP/IP socket
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

# Bind the socket to the port
server_address = ('localhost', 10000)
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


# Create a UDP clisocket
clisock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

server_address = ('localhost', 10000)
message = 'This is the message.  It will be repeated.'

try:

    # Send data
    print ('sending "%s"' % message)
    sent = clisock.sendto(message, server_address)

    # Receive response
    print ('waiting to receive')
    data, server = clisock.recvfrom(4096)
    print ('received "%s"' % data)

finally:
    print ('closing clisocket')
    clisock.close()