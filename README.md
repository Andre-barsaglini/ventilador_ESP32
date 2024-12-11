# Controle remoto do ventilador do Túnel de Vento

## Informações importantes

#### conectividade
* IP: 162.168.0.169
* Porta:6969


#### pinos
* liga e desliga o ventilador: 17
* saidas digitais: 4,2,15


#### componentes
* ESP32 
* ULN2003 (saidas digitais)
* MCP41010 (pot digital)

#### comandos

* "a\r" - configura o microcontrolador para funcionar de forma analogica (pot). O ESP responde 2 se ok. 
* "d\r" - configura o microcontrolador para funcionar de forma digital (multispeed). o ESP responde 2 se ok.
* "r\r" - run (liga o ventilador). o ESP responde 5 se ok.
* "s\r" - stop (desliga o ventilador). o ESP responde 4 se ok.
* no modo digital: valores entre 1 e 8 selecionam as velocidades correspondentes. o ESP responde 0 se ok.
* no modo analogico: valores entre 1 e 256 selecionam o valor do cursor do potenciometro. o ESP responde 0 se ok.
* qualquer comando diferente desses o ESP responde 1
  
## Descrição do protótipo
O controle funciona atuando as entradas digitais do CFW09 utilizando o recurso multispeed do inversor ou por meio de um potenciômetro digital de 8 bits instalado na entrada analógica do inversor (o controle por potenciometro ainda não foi implementado e testado, cuidado ao utilizar). A comunicação com o ESP32 é feita por TCP enviando comandos ao microcontrolador, que por sua vez responde via tcp. UDP é utilizado somente no upload do código do ESP32 via wireless.

O protótipo está instalado em uma caixa patola preta no trilho din dentro do painel e tem a etiqueta controle do inversor. 

Caso a intenção seja concluir a implementação do controle analógico é interessante instalar um ADC para leitura da saída analógica do inversor.  

## Funcionamento e código
O ESP roda um setup para configurar os pinos e a conectividade, desativa a task de loop e roda duas tasks, uma para checar periodicamente a conectividade e outra para aguardar a mensagem via socket. Quando uma mensagem é recebida e reconhecida ele dispara uma outra taks para efetuar a atuação conforme o comando recebido, e após a conclusão encerra a task. Existe a opção de encerrar também o socket após recebida a mensagem, o que pode ser configurado na variavel closeAfterRec.

