# https://pymotw.com/2/socket/udp.html

import socket
import sys

map_size = 5
ips = [('localhost',4567),('localhost',4568)]
const = 4096
bastao = False

def passabastao():
	global bastao
	bastao = not bastao

def client(jogador):
	# Create a UDP clisocket
	clisock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

	server_address = ips[(jogador + 1) % len(ips)]
	print server_address
	targ = raw_input('Qual mapa deseja atacar? ')
	xy = raw_input('Qual coordenada desse mapa? ')

	message = targ + '-' + xy
	try:

	    # Send data
	    print ('sending "%s"' % message)
	    sent = clisock.sendto(message, server_address)

	    # Receive response
	    print ('waiting to receive')
	    data, server = clisock.recvfrom(const)
	    print ('received "%s"' % data)

	finally:
	    print ('closing clisocket')
	    clisock.close()
	    passabastao()

def server(jogador):
	# Create a TCP/IP socket
	sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

	# Bind the socket to the port
	server_address = ips[jogador]
	print ('starting up server on %s port %s' % server_address)
	sock.bind(server_address)

	if True:
	    print ('\nwaiting to receive message')
	    data, address = sock.recvfrom(4096)
	    
	    print ('received %s bytes from %s' % (len(data), address))
	    print data
	    
	    if data:
	        sent = sock.sendto(data, address)
	        print ('sent %s bytes back to %s' % (sent, address))
	    passabastao()

if (len(sys.argv) != 2):
	print("Por favor, digite o numero informado a voce pelo administrador")
	sys.exit(0)
jogador = int(sys.argv[1])

print("Ola! Bem-vindo ao jogo batalha naval!\nConheca o seu mapa:\n 01 02 03 04 05 \n 06 07 08 09 10 \n 11 12 13 14 15\n 16 17 18 19 20 \n 21 22 23 24 25\n")

nav1 = raw_input("Digite tres numeros distintos de 0 a %d para posicionar seus navios no mapa:\n" % (map_size*map_size))
nav2 = raw_input()
nav3 = raw_input()

bastao = (jogador == 0)
while True:
	if (bastao):
		client(jogador)
	else: 
		server(jogador)
sys.exit(0)