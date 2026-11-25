# Garra hidráulica com ESP32

## Gravar e abrir o painel

1. Extraia o ZIP. Na Arduino IDE, abra `garra_esp32/garra_esp32.ino`. Mantenha `Painel.h` na mesma pasta. Esse arquivo contém o HTML, o CSS e o JavaScript que serão gravados com o firmware.
2. No Gerenciador de Placas, instale **esp32 by Espressif Systems** (versão testada: **3.3.7**). Selecione a placa e a porta USB correspondentes ao seu ESP32 e clique em **Carregar**. Para um DevKit com ESP32 clássico, a opção genérica é **ESP32 Dev Module**.
3. Conecte o celular ou computador ao Wi-Fi **Garra-ESP**, com a senha **garra1234**. Se o aparelho avisar que a rede está sem internet, escolha permanecer conectado.
4. Abra **http://192.168.4.1** no navegador. Use um toque longo, clique mantido ou mantenha **Espaço/Enter** pressionado sobre um botão de movimento. Solte para parar. O botão vermelho interrompe os quatro e cancela os comandos anteriores de movimento.

O ESP32 cria a rede e entrega a página pelo endereço acima. A interface não precisa de internet, CDN ou gravação separada em SPIFFS/LittleFS. A instalação inicial do suporte à placa pode exigir internet no computador.

## Pinagem preservada

| Controle | Botão − | Botão + | Entrada A | Entrada B |
| --- | --- | --- | --- | --- |
| Base | Esquerda | Direita | GPIO 32 | GPIO 33 |
| Ombro | Descer | Subir | GPIO 25 | GPIO 26 |
| Cotovelo | Recolher | Estender | GPIO 23 | GPIO 22 |
| Garra | Abrir | Fechar | GPIO 19 | GPIO 18 |

Esses GPIOs são sinais para a ponte H da montagem existente. Confira os sentidos inicialmente com pulsos curtos e o mecanismo livre para se movimentar. Se algum estiver invertido, mude o respectivo `false` para `true` na tabela `MOTORES` do sketch.

A velocidade ajusta o PWM de **30% a 100%**, com padrão de **70%**. É um ajuste de potência aplicada à ponte H. A velocidade física depende do motor, da ponte e da carga.

## Comportamento de parada

- Ao soltar o último botão, o painel envia uma parada prioritária. Se houver outros botões pressionados, o painel atualiza quais juntas devem continuar.
- Enquanto houver movimento, o painel renova o comando aproximadamente a cada **110 ms**. Ao perder essa renovação durante **400 ms**, o firmware zera todas as saídas de PWM na sua tarefa de **10 ms**, independente do processamento HTTP.
- Após **6 s** de movimento contínuo em uma junta, o firmware interrompe essa junta. Solte o botão para liberar um novo acionamento.
- O ESP32 rejeita comandos de movimento anteriores a uma parada geral ou perda de renovação. O painel também libera os botões ao perder o foco ou a conexão.
- O botão vermelho é uma parada por software. O limite de 6 s mede tempo e não detecta o fim de curso físico da seringa.

Estrutura de rede e pinagem baseadas no firmware enviado e no [repositório de referência](https://github.com/jao0097/pfinal).
