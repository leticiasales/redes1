# https://pymotw.com/2/socket/udp.html

import socket
import json
import sys
from struct import *

#defstructmensagem
#ou eh "to o bastao" (cod)
#ou eh "passa pra frente" (cod, alvo, coor, alvoleu, acertou/errou)

map_size = 5
ips = [('localhost',4567),('localhost',4568),('localhost',4569),('localhost',4570)]
const = 4096
bastao = False
meunumero = False

tipojogada = 0
tipobastao = 1
tipoaviso = 2

def conf_client():
	global meunumero
	# Create a UDP clisocket
	clisock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

	server_address = ips[(meunumero + 1) % len(ips)]
	print server_address
	return clisock, server_address

def conf_server():
	global meunumero
	sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

	# Bind the socket to the port
	server_address = ips[meunumero]
	print ('starting up server on %s port %s' % server_address)
	sock.bind(server_address)
	return sock

def tem_bastao(clisock, server_address):
	global bastao, meunumero, first, tipobastao
	if (not first):
		data, address = sock.recvfrom(const)
		sender = int(data[4])
		# print ('enviado por "%d"' % sender)		
		# print ('recebido por "%d"' % meunumero)	
		if (meunumero == sender):
			bastao = False
			first = True
			destino = (meunumero + 1) % len(ips)
			message = (tipobastao, meunumero, destino)
			sent = clisock.sendto(str(message), server_address)
			return
	# print ('first')
	first = False	
	targ = int(raw_input('Qual mapa deseja atacar? '))
	xy = int(raw_input('Qual coordenada desse mapa? '))

	message = (tipojogada, meunumero, targ, xy, 0)

	# Send data
	# print ('sending "%s"' % (message,))
	sent = clisock.sendto(str(message), server_address)

	# Receive response
	# print ('waiting to receive')
	# @@timeout tem_bastao(clisock, server_address)
	# data, server = clisock.recvfrom(const)
	# print ('received "%s"' % data)

def passa_pra_frente(clisock, server_address, message):
	sent = clisock.sendto(str(message), server_address)

if (len(sys.argv) != 2):
	print("Por favor, digite o numero informado a voce pelo administrador")
	sys.exit(0)
meunumero = int(sys.argv[1])

# print("Ola! Bem-vindo ao jogo batalha naval!\nConheca o seu mapa:\n")
# print("01 02 03 04 05 \n 06 07 08 09 10 \n 11 12 13 14 15\n 16 17 18 19 20 \n 21 22 23 24 25\n")

# nav = (raw_input("Digite dois numeros distintos de 0 a %d para posicionar seus navios no mapa:\n" % (map_size*map_size)),raw_input())
first = True
bastao = (meunumero == 0)
clisock, server_address = conf_client()
sock = conf_server()

while (True):
	if (bastao):
		tem_bastao(clisock, server_address)
	else: 
		# print ('\nwaiting to receive message')
		data, address = sock.recvfrom(const)
		# print ('received %s bytes from %s' % (len(data), address))
		# print data
		if data:
			# print data
			tipo = int(data[1])
			# print tipo
			origem = int(data[4])
			# print origem
			destino = int(data[7])
			# print destino
			# if (targ == origem):
				# print 'OUCH'
			if (tipo == tipojogada):
				if (destino==meunumero):
				# @@aqui vai verificar o mapa e responder se acertou
				passa_pra_frente(clisock, server_address, data)
			if (tipo==tipobastao):
				# print destino
				# print meunumero
				if (destino==meunumero):
					bastao = True
			# sent = sock.sendto(data, address)
			# print ('sent %s bytes back to %s' % (sent, address))
print ('closing clisocket')
clisock.close()
sys.exit(0)