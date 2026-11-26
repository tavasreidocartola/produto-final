# Garra hidráulica com ESP32

O painel web controla os dois motores do braço. A base e a garra são operadas manualmente.

| Motor | Controles | GPIOs do ESP32 | ID em `/cmd` |
| --- | --- | --- | --- |
| Ombro | Descer / Subir | 25 e 26 | 1 |
| Cotovelo | Recolher / Estender | 23 e 22 | 2 |

## Gravar o firmware e abrir o painel

1. Baixe ou clone o projeto e abra `garra_esp32/garra_esp32.ino` na Arduino IDE. O HTML, o CSS e o JavaScript já estão no sketch.
2. Selecione a placa e a porta USB e clique em **Carregar**.
3. Conecte seu celular ou computador à rede **Garra-Carangueijo** e abra **http://192.168.4.1**. Se aparecer o visual antigo, recarregue a página.

Segure um botão para mover e solte para parar. O botão vermelho para os dois motores. O painel mantém o ajuste de velocidade por PWM e o firmware interrompe o movimento quando deixa de receber os comandos de renovação.

A interface funciona pela rede do ESP32 usando a rota `/cmd?c=`. Projeto baseado no [firmware de referência](https://github.com/jao0097/pfinal).
