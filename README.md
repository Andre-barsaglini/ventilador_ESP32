# Controle remoto do ventilador do Túnel de Vento

## Informações importantes
### conectividade
IP: 162.168.0.169
Porta:6969
### pinos
liga e desliga o ventilador: 17
saidas digitais: 4,2,15
### componentes
ESP32 
ULN2003 (saidas digitais)
MCP41010 (pot digital)

## Descrição do protótipo
O controle funciona atuando as entradas digitais do CFW09 utilizando o recurso multispeed do inversor ou por meio de um potenciômetro digital de 8 bits instalado na entrada analógica do inversor (o controle por potenciometro ainda não foi implementado e testado, cuidado ao utilizar). A comunicação com o ESP32 é feita por TCP enviando comandos ao microcontrolador, que por sua vez responde via tcp. UDP é utilizado somente no upload do código do ESP32 via wireless.

O protótipo está instalado em uma caixa patola preta no trilho din dentro do painel e tem a etiqueta controle do inversor. 

## Funcionamento e código
O ESP roda tasks separadas para cada coisa. No setup 
