🇺🇸 [Read this in English](README.md) | 🇧🇷 Leia em Português

---

# 🚨 Assistente de Cuidado IoT: Sensor de Queda Vestível

Este repositório contém o firmware (C++) desenvolvido para um protótipo de dispositivo vestível focado na segurança e monitoramento de idosos. O projeto foi estruturado utilizando um microcontrolador ESP32 e um acelerômetro MPU-6050, com o objetivo de detectar quedas, enviar alertas remotos e avaliar o nível de sedentarismo do usuário, preservando a sua privacidade (sem o uso de câmeras).

## ⚙️ Principais Funcionalidades

*   **Detecção de Quedas (Lógica Difusa):** Análise da magnitude euclidiana e detecção de queda livre seguida de impacto. Utiliza Lógica Fuzzy (Modelo de Sugeno) para calcular a probabilidade da queda e mitigar falsos positivos.
*   **Alerta Sonoro e Cancelamento Local:** Emissão de bip local (Buzzer) com uma janela de 10 segundos para o usuário cancelar o alarme através de um sensor touch, evitando envios indevidos à nuvem.
*   **Botão SOS (Pânico):** Acionamento manual de emergência a qualquer momento pelo usuário.
*   **Telemetria via Telegram (Long Polling):** Comunicação bidirecional. O sistema envia alertas automáticos e responde a comandos manuais (como `/status`) via Telegram API.
*   **Dashboard Web Local:** Servidor assíncrono hospedado no próprio ESP32, que expõe uma rota JSON atualizada via `fetch()` no front-end para visualização de métricas em tempo real.
*   **Monitoramento de Sedentarismo:** Máquina de Estados Finita (FSM) que avalia o tempo ininterrupto de repouso e emite alertas progressivos (Leve, Moderado, Grave).

## 🛠️ Hardware Utilizado

*   **ESP32:** Processamento principal e comunicação Wi-Fi nativa.
*   **MPU-6050:** Sensor inercial (Acelerômetro e Giroscópio de 6 Eixos).
*   **TTP223B:** Sensor Touch Capacitivo para interação do usuário.
*   **Buzzer:** Módulo de alerta sonoro local.

## 🏗️ Arquitetura de Software

O firmware foi codificado em **C++** (Arduino IDE) e desenhado para ser totalmente assíncrono. O uso de funções bloqueantes (como `delay()`) foi substituído por lógicas baseadas em `millis()`, permitindo que o ESP32 execute de forma paralela:
1. A leitura inercial a 100Hz.
2. O servidor Web local.
3. A comunicação com a API do Telegram (Long Polling).
4. O monitoramento tátil do botão SOS (com debounce de retenção).

---

## 🤖 Uso de Inteligência Artificial no Desenvolvimento

Este projeto foi desenvolvido como um protótipo acadêmico com prazos rigorosos. Para viabilizar a entrega de toda a parte de software a tempo, utilizei IA generativa (vibe coding) para acelerar a escrita da estrutura base do código em C++.

**Meu papel principal na engenharia deste código consistiu em:**
*   Definir a arquitetura geral do sistema (loop assíncrono, FSM de sedentarismo e a implementação matemática da Lógica Difusa).
*   Revisar criticamente a lógica gerada e debugar os erros de integração.
*   Conectar os conceitos lógicos de software com o comportamento físico do hardware (calibração do MPU-6050 e pinagem do ESP32).

Como a codificação bruta foi assistida por IA, o foco deste repositório é demonstrar a **arquitetura**, a **visão de produto** e a **prova de conceito** do funcionamento integrado de IoT, e não necessariamente otimizações de micro-nível em cada linha de código.

---

## 👥 Equipe de Desenvolvimento

Projeto desenvolvido para a disciplina de Soluções para Desafios em Engenharia (UFABC) por uma equipe multidisciplinar (Ciência da Computação, Engenharia Aeroespacial e Engenharia Biomédica):
*   Pedro A. S. Lopes (Desenvolvimento do Firmware e IoT)
*   Marcelo R. Ahagon
*   Felipe A. R. Caramante
*   Guilherme A. M. Almendra
