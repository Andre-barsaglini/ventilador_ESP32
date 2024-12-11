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
* no modo digital: valores entre 1 e 8 + "\r" selecionam as velocidades correspondentes. o ESP responde 0 se ok.
* no modo analogico: valores entre 1 e 256 + "\r" selecionam o valor do cursor do potenciometro. o ESP responde 0 se ok.
* qualquer comando diferente desses o ESP responde 1
  
## Descrição do protótipo
O controle funciona atuando as entradas digitais do CFW09 utilizando o recurso multispeed do inversor ou por meio de um potenciômetro digital de 8 bits instalado na entrada analógica do inversor (o controle por potenciometro ainda não foi implementado e testado, cuidado ao utilizar). A comunicação com o ESP32 é feita por TCP enviando comandos ao microcontrolador, que por sua vez responde via tcp. UDP é utilizado somente no upload do código do ESP32 via wireless.

O protótipo está instalado em uma caixa patola preta no trilho din dentro do painel e tem a etiqueta controle do inversor. 

Caso a intenção seja concluir a implementação do controle analógico é interessante instalar um ADC para leitura da saída analógica do inversor.  

## Funcionamento, código e considerações
O ESP roda um setup para configurar os pinos e a conectividade, desativa a task de loop e roda duas tasks, uma para checar periodicamente a conectividade e outra para aguardar a mensagem via socket. Quando uma mensagem é recebida e reconhecida ele dispara uma outra taks para efetuar a atuação conforme o comando recebido, e após a conclusão encerra a task. Existe a opção de encerrar também o socket após recebida a mensagem, o que pode ser configurado na variavel closeAfterRec.

O core onde rodam as tasks que não são de comunicação pode ser alterado. Recomendo cautela com relação a isso por dois motivos. Primeiro que algumas coisas precisam rodar em cores específicos em função das bibliotecas que utilizam e segundo que blocar ou travar o core que roda o wifi ou o update OTA pode causar problemas. 

Basicamente a operação é feita por socket tcp e qualquer cliente deve bastar conectando na rede do tunel, informando IP e porta. Um ponto de atenção pode ser a quebra de linha, não lembro o motivo de ter utilizado só \r, no que testei está funcionando. Para habilitar o controle é necessário apertar o botao REM na interface do inversor. para voltar a controle via HMI é necessário apertar novamente. Isso póde ser implementado vai hardware no futuro

As credenciais do WIFI estão num arquivo separado, criar manualmente.

Quanto a implementação via controle analógico, ressalto a importancia do cuidado, pois no processo é possivel que o ventilador seja acionado em cem porcento, coisa que eu nunca testemunhei e nem quero. Pode ser interessante reconfigurar o valor da velocidade maxima durante os testes preliminares para evitar esse problema, e depois que o sistema estiver previamente ajustado (com os parametros do inversor certos para esse ou para outro potenciometro) retornar com o valor origianal. O pot de 8 bits em teoria deve dar uma resolução proxima de 2rpm por divisão, o que deve bastar. É necessário estudar o desemprenho do potenciometro com realção a ruidos, imagino que deve ser bom, mas não sei. No controle de corrente da anemometria acredito que o rapazote configurou no inversor um filtro ou um amortecimento bem alto com a intenção de tentar garantir uma estabilidade maior. isso impacta bastante no tempo de estabilização do sistema a cada alteração de rotação. Via de regra, pra 99% do serviço que eu vi acontecer no túnel o controle digital é muito melhor. Nunca vi precisar de mais de 8 velocidades num único ensaio, e se precisar de velocidades específicas é só alterar o parametro na interface, leva um minuto. 
