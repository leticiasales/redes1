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
dead = {}

def conf_client():
	global meunumero
	# Create a UDP clisocket
	clisock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

	server_address = ips[(meunumero + 1) % len(ips)]
	# print server_address
	return clisock, server_address

def conf_server():
	global meunumero
	sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

	# Bind the socket to the port
	server_address = ips[meunumero]
	# print ('starting up server on %s port %s' % server_address)
	print ('Aguarde a sua vez.')
	sock.bind(server_address)
	return sock

def tem_bastao(clisock, server_address):
	global bastao, meunumero, first, tipobastao
	if (not first):
		data, address = sock.recvfrom(const)
		message = data[1:-1].split(',')
		sender = int(message[1])
		# print ('enviado por "%d"' % sender)		
		# print ('recebido por "%d"' % meunumero)	
		if (meunumero == sender):
			acertou = message[4]
			if (acertou == 'True'):
				print('Voce destruiu um navio inimigo. Aguarde sua proxima jogada.')
			else:
				print ('Ataque enviado. Aguarde a sua vez.')
			passa_o_bastao()
			return
	# print ('first')
	print('Sua vez!')
	first = False	
	targ = int(raw_input('Qual mapa deseja atacar? '))
	xy = int(raw_input('Qual coordenada desse mapa? (De 1 a 25) '))

	message = (tipojogada, meunumero, targ, xy, False)

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

def passa_o_bastao():
	global bastao, meunumero, first, tipobastao
	bastao = False
	first = True
	destino = (meunumero + 1) % len(ips)
	message = (tipobastao, meunumero, destino)
	# print message
	sent = clisock.sendto(str(message), server_address)

if (len(sys.argv) != 2):
	print("Por favor, digite o numero informado a voce pelo administrador")
	sys.exit(0)

meunumero = int(sys.argv[1])

print("Ola! Bem-vindo ao jogo batalha naval!\nConheca o seu mapa:\n")
print(" 01 02 03 04 05 \n 06 07 08 09 10 \n 11 12 13 14 15 \n 16 17 18 19 20 \n 21 22 23 24 25 \n")

# for x in range(25):
	# print ('0' + str(x))[-2:],

meumapa = [('0' + str(i))[-2:] for i in range(26)]

#nav1, nav2 = (raw_input("Digite dois numeros distintos de 0 a %d para posicionar seus submarinos no mapa:\n" % (map_size*map_size)),raw_input())

#meumapa[int(nav1)] = 1
#meumapa[int(nav2)] = 1

a, b, c, d, e, f = (raw_input("Escolha duas vezes três casas adjacentes de 0 a %d para inserir os dois barcos no mapa \n" % (map_size*map_size)),raw_input(),raw_input(),raw_input(),raw_input(),raw_input())
while ((d==a) ou (d==b) ou (d==c) ou (e==a) ou (e==b) ou (e==c) ou (f==a) ou (f==b) ou (f==c) ou (b!=a+1 e b!=a-1 e b!=a+5 e b!=a-5) ou (c!=b+1 e c!=b-1 e c!=b+5 e c!= b-5) ou (e!=d+1 e e!=d-1 e e!=d+5 e e!=d-5) ou (f!=e+1 e f!=e-1 e f!=e+5 e f!=e-5)):
		a, b, c, d, e, f = (raw_input("Coordenadas inválidas, escolha novamente \n" % (map_size*map_size)),raw_input(),raw_input(),raw_input(),raw_input(),raw_input());
meumapa[int(a)] = 1
meumapa[int(b)] = 1
meumapa[int(c)] = 1
meumapa[int(d)] = 1
meumapa[int(e)] = 1
meumapa[int(f)] = 1

first = True
bastao = (meunumero == 0)
clisock, server_address = conf_client()
sock = conf_server()
navios = 2 #chuncho

while (len(dead)<3):
	if (bastao and (navios > 0)):
		tem_bastao(clisock, server_address)
	else: 
		# print ('\nwaiting to receive message')
		data, address = sock.recvfrom(const)
		# print ('received %s bytes from %s' % (len(data), address))
		# print data
		if data:
			message = data[1:-1].split(',')
			# print message
			tipo = int(message[0])
			# print tipo
			origem = int(message[1])
			# print origem
			destino = int(message[2])
			# print destino

			if (tipo == tipojogada):
				if (destino==meunumero):
					coord = int(message[3])
					# print coord
					# @@aqui vai verificar o mapa e responder se acertou
					if (meumapa[coord] == 1):
						print 'OUCH'
						navios-=1
						meumapa[coord] = 0
						if (navios == 0):
							print ('Todos seus submarinos afundaram. Aguarde o fim do jogo.')
				passa_pra_frente(clisock, server_address, data)

			if (tipo==tipobastao):
				# print destino
				# print meunumero
				if (destino==meunumero):
					bastao = True
					if (navios == 0):
						message = (tipoaviso, meunumero, meunumero)
						sent = clisock.sendto(str(message), server_address)
			if (tipo == tipoaviso):
				# print ('fui avisado')
				dead[destino] = True	
				if (destino==meunumero):
					passa_o_bastao()
				else:
					passa_pra_frente(clisock, server_address, data)
			# sent = sock.sendto(data, address)
			# print ('sent %s bytes back to %s' % (sent, address))
# print ('closing clisocket')
if (not (meunumero in dead)):
	print ('Parabéns! Você derrubou todos os submarinos inimigos.')
else:
	print ('Fim de jogo.')
clisock.close()
sys.exit(0)