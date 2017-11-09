# https://pymotw.com/2/socket/udp.html

import socket
import json
import sys

map_size = 5
ips = [('localhost',4567),('localhost',4568),('localhost',4569),('localhost',4570)]
const = 4096
bastao = False
jogador = False

def passabastao():
	global bastao
	bastao = not bastao

def conf_client():
	global jogador
	# Create a UDP clisocket
	clisock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

	server_address = ips[(jogador + 1) % len(ips)]
	print server_address
	return clisock, server_address

def conf_server():
	global jogador
	sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

	# Bind the socket to the port
	server_address = ips[jogador]
	print ('starting up server on %s port %s' % server_address)
	sock.bind(server_address)
	return sock

def tem_bastao(clisock, server_address):
	global bastao
	global jogador
	targ = raw_input('Qual mapa deseja atacar? ')
	xy = raw_input('Qual coordenada desse mapa? ')

	message = (targ, xy, jogador)
	# Send data
	print ('sending "%s"' % (message,))
	sent = clisock.sendto(str(message), server_address)

	# Receive response
	print ('waiting to receive')
	data, server = clisock.recvfrom(const)
	print ('received "%s"' % data)

	bastao = False


if (len(sys.argv) != 2):
	print("Por favor, digite o numero informado a voce pelo administrador")
	sys.exit(0)
jogador = int(sys.argv[1])

print("Ola! Bem-vindo ao jogo batalha naval!\nConheca o seu mapa:\n 01 02 03 04 05 \n 06 07 08 09 10 \n 11 12 13 14 15\n 16 17 18 19 20 \n 21 22 23 24 25\n")

nav = (raw_input("Digite tres numeros distintos de 0 a %d para posicionar seus navios no mapa:\n" % (map_size*map_size)),raw_input(),raw_input())


bastao = (jogador == 0)
clisock, server_address = conf_client()
sock = conf_server()

while True:
	if (bastao):
		print bastao
		print 'client'
		tem_bastao(clisock, server_address)
	else: 
		print bastao
		print 'server'

		print ('\nwaiting to receive message')
		data, address = sock.recvfrom(4096)

		print ('received %s bytes from %s' % (len(data), address))
		print data

		if data:
			data = data[1:-1]
			nextbastao = int(data[-1]) + 1
			if (jogador == (nextbastao % len(ips))):
				bastao = True
				print 'oi'
			# aqui vai verificar o mapa e responder se acertou
			sent = sock.sendto(data, address)
			print ('sent %s bytes back to %s' % (sent, address))
print ('closing clisocket')
clisock.close()
sys.exit(0)