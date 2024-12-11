# Controle remoto do ventilador do Túnel de Vento

##Descrição do protótipo
O controle funciona atuando as entradas digitais do CFW09 utilizando o recurso multispeed do inversor ou por meio de um potenciômetro digital instalado na entrada analógica do inversor (o controle por potenciometro ainda não foi implementado e testado, cuidado ao utilizar). A comunicação com o ESP32 é feita por TCP, inclusive está implementado o update de firmware wireless, evitando a necessidade de remover o dispositivo para reprogramação. 
