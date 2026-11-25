# Garra hidráulica com ESP32

Este projeto usa o firmware original da garra com uma interface web redesenhada. O sketch contém a página em HTML, CSS e JavaScript no bloco `PAGINA_HTML`.

## Atualizar a interface

1. Se você já tem o sketch que funciona, abra os dois arquivos e copie apenas o bloco `const char PAGINA_HTML[]` do novo para o seu, da declaração até `)rawliteral";`. Isso preserva qualquer ajuste que você tenha feito na sua cópia.
2. Se preferir usar o arquivo completo, abra `garra_esp32/garra_esp32.ino` na Arduino IDE. O arquivo foi montado com as configurações do código enviado, incluindo o nome da rede **Garra-Carangueijo**.
3. Na Arduino IDE, selecione sua placa e a porta USB e clique em **Carregar**. A página fica na memória do ESP32, então a gravação atualiza a página que ele serve.
4. Conecte-se à rede **Garra-Carangueijo** e abra **http://192.168.4.1**. Se vir o visual antigo, recarregue a página.

Segure um botão para mover e solte para parar. O botão vermelho aciona a parada geral. O controle de velocidade continua usando o PWM do firmware original.

O painel usa a rota original `/cmd?c=` e funciona pela própria rede Wi-Fi do ESP32. O ponto de acesso, a pinagem, os comandos e a lógica de parada do firmware vêm do anexo e do [repositório de referência](https://github.com/jao0097/pfinal).
