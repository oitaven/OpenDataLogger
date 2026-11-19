# OpenDataLogger — Arduino UNO R4 WiFi

**Versão experimental 1.0.2 · MIT · interface Web local · aquisição prioritária**

> **Estado de validação:** existem ensaios em navegador e testes C++ simulados, bem como utilização experimental numa UNO R4 WiFi. Ainda não foi executada nesta entrega uma compilação completa automatizada no núcleo Renesas, nem validado o microSD físico. A publicação inclui um workflow GitHub Actions para iniciar essa verificação. **Não utilizar como dispositivo de segurança ou de controlo de equipamento crítico.**

## Começar

1. Instalar o suporte **Arduino UNO R4 Boards** (testado em desenvolvimento com a série 1.6.x) e seleccionar **UNO R4 WiFi**.
2. Copiar `arduino_secrets.h.example` para `arduino_secrets.h`, preencher SSID, passwords e o código de administração. O ficheiro real é excluído do Git.
3. Abrir `OpenDataLogger.ino` no Arduino IDE, **Verify** e **Upload**; para ensaios sem sensores, activar **DEMO** na Monitorização.
4. Consultar `GUIA_RAPIDO_1.0.2.md`, `TEST_PLAN_1.0.2.md` e `SECURITY.md`.

**Matriz LED:** o sinal simulado A1 tem período de 9,5 s. Com uma janela de 30 s, 12 colunas equivalem a 2,5 s/coluna, comprimindo cerca de três ciclos; a janela de **10 s** mostra uma oscilação com melhor resolução. A matriz de 12×8 LEDs não reproduz graficamente a suavidade da Web.

**Código-fonte:** MIT, ver `LICENSE`. Dependências de Arduino são instaladas pelo gestor de placas/bibliotecas e mantêm as suas licenças. Para publicar consultar `PUBLICACAO_GITHUB.md`.

---


**Projecto genérico:** nomes predefinidos `A0`–`A5`, `D2` e `D3` (sem assumir a aplicação de cada canal), configuráveis na Web. O exemplo do frigorífico é apenas um cenário de ensaio; nenhuma regra interna assume que D2 ou D3 tenham um significado específico.

Data logger com Arduino UNO R4 WiFi, aquisição analógica prioritária, detecção de flancos digitais, matriz LED 12×8 e painel Web local. Esta versão agrega funcionalidades de análise e sessões; **não está ainda homologada nem validada no hardware real**. Guardar a pasta 0.4.7 antes de actualizar.

## Instalação no Arduino IDE 2

1. Abrir `OpenDataLogger/OpenDataLogger.ino` (os ficheiros `WebPage.ino`, `AdvancedFeatures.h` e `OptionalSD.h` devem estar na mesma pasta).
2. Instalar/seleccionar `Arduino UNO R4 WiFi` e o respectivo núcleo Renesas; a versão de referência para este código é a 1.6.0.
3. Editar **apenas no ficheiro `arduino_secrets.h`** as redes iniciais, palavra-passe forte do Access Point, e **`ODL_WEB_ADMIN_KEY`** (mínimo 12 caracteres; não deixar `ALTERAR_CODIGO_ADMIN_FORTE`). Estes dados **nunca** devem ser publicados no Git.
4. Compilar com **Verify**, carregar com **Upload** e observar o Serial Monitor a 115200 baud. Abrir o IP indicado no navegador. Em modo AP, ligar à rede configurada no firmware e visitar o IP indicado.
5. Começar com `Simulação` activa, quatro canais, intervalo de 100 ms e janela da matriz de 10 s. Em **Sistema**, observar `perdas FIFO`, `desvio IRQ` e `latência HTTP`.

**Migração de 1.0.0:** se conservares um `arduino_secrets.h` antigo sem `WEB_ADMIN_KEY`, o erro de compilação foi corrigido. O sketch utiliza agora a macro `ODL_WEB_ADMIN_KEY`. Para activar alterações Wi-Fi pela Web, acrescenta `#define ODL_WEB_ADMIN_KEY "uma-chave-pessoal-bem-longa"` ao teu `arduino_secrets.h`. Sem esta macro, o código compila e só a alteração Wi-Fi pela Web fica desactivada. Uma variável antiga `const char* WEB_ADMIN_KEY` pode ser eliminada; não é usada. As configurações e nomes já guardados na EEPROM mantêm-se: usa **Canais → Repor nomes genéricos**, sai do separador e guarda a configuração se desejares trocar os nomes antigos.

**Compatibilidade:** o mecanismo de aquisição continua a executar `analogRead()` dentro de uma interrupção `FspTimer`. Passou testes C++ com mocks, mas necessita de verificação na UNO R4 real antes de se considerar fiável para medições críticas. O núcleo Renesas e a biblioteca WiFiS3 podem diferir entre versões.

## Funcionalidades

| Área | Implementação nesta versão |
|---|---|
| Aquisição | A0–A5 sincronizados num temporizador comum (100 ms a 10 s); FIFO de 64 posições; contador de perdas e *jitter* |
| Digital | D2/D3 por interrupções CHANGE, 128 transições em memória circular; simulação temporal independente da amostragem ADC |
| Web | Binário incremental ODLB v2, cinco modos de gráfico, metadados dos testes, matriz LED de 1/3/10/30 s |
| Cursores | Dois instantes no gráfico: Δt e diferença dos canais visíveis; disponíveis na Monitorização |
| Calculados | Quatro canais virtuais: diferença, soma, razão, derivada e média móvel; cálculos no navegador, configuração no Arduino |
| Alarmes | Quatro regras no Arduino, limiares, duração mínima, ON/OFF, delta e condição composta; registar ou disparar trigger armado |
| NTC | Até três pontos: 1–2 pontos corrigem Beta; 3 pontos calculam Steinhart–Hart; guardar na EEPROM |
| Sessões | Gravação temporária na memória do Safari; exportação JSON/CSV, importação JSON/CSV microSD, reprodução e comparação entre duas sessões |
| Ciclos | Por canal digital: contagem de flancos ON, tempo ON completo e duração média; por canal analógico: mínimo/máximo; exportação CSV |
| Wi-Fi | Configuração pela página **Sistema**, protegida pelo código de administração; gravação EEPROM, aplicada após reiniciar |
| microSD | Suporte SPI compilado; detecção no arranque, início automático de sessão quando há cartão, CSV com amostras e eventos; sem cartão: RAM/Safari |

Os cálculos no Safari não entram nas interrupções nem alteram as amostras originais. Os alarmes são avaliados no ciclo principal quando as amostras são processadas; um atraso de comunicação pode retardar um alarme, apesar de a IRQ continuar a adquirir. Triggers compostos estão disponíveis como acção de uma regra, **apenas quando um trigger está armado**.

## Sessões no navegador

1. Em `Sessões`, dar nome e seleccionar **Iniciar**. Enquanto a página estiver aberta, ela acumula amostras e flancos além dos 4 KB de memória circular da placa.
2. **Parar** e **Exportar sessão JSON** para guardar toda a experiência. Também existe CSV de amostras brutas e CSV de impulsos.
3. Escolher um JSON previamente exportado, ou um CSV proveniente do módulo SD, para reproduzir e comparar com uma sessão actual. O comparador alinha pelo início ou pelo primeiro flanco digital de subida disponível.
4. Os limites na memória do navegador são 100 000 amostras e 50 000 eventos por sessão; quando o limite é atingido ou existem lacunas na transmissão, a interface assinala a situação.

Sem microSD, **fechar/actualizar a página implica perder a sessão ainda não exportada**. Uma desconexão superior à retenção do ring buffer pode perder amostras; o Safari não inventa os pontos em falta. Em ficheiros SD importados, verificar os parâmetros e a calibração do sensor, pois pode haver diferenças entre experiências.

## Wi-Fi a partir da Web

Em `Sistema → Configuração Wi-Fi`, preencher SSID principal/secundário, deixar palavras-passe vazias para as manter ou introduzir novas, indicar nome do AP e o código `ODL_WEB_ADMIN_KEY` definido no firmware. Clicar `Guardar Wi-Fi na EEPROM` e **reiniciar o Arduino**. Ao mudar de rede, o IP poderá mudar. As palavras-passe nunca são enviadas de volta à página.

**Segurança:** o HTTP não usa TLS; palavras-passe e código de administração são enviados sem cifragem a nível da aplicação. Usar apenas em Wi-Fi de confiança ou no AP privado protegido. Não expor a porta 80 à Internet. As credenciais ficam na EEPROM, não cifradas contra acesso físico. Os restantes endpoints de configuração também não têm autenticação (arquitectura de protótipo).

## microSD — detecção automática no arranque

**Não ligar directamente um cartão microSD de 3,3 V aos sinais SPI de 5 V da UNO R4.** Usar módulo/adaptador com alimentação e adaptação lógica adequadas. Ligações usuais: MOSI D11, MISO D12, SCK D13, CS D10, GND comum, alimentação conforme o módulo. Confirmar a pinagem específica do hardware antes de ligar.

**Não é necessário recompilar quando ligares o leitor.** A partir desta versão o suporte SD é incluído por omissão (`ODL_ENABLE_SD 1`). Em cada arranque, o Arduino tenta `SD.begin(ODL_SD_CS_PIN)`; se não encontrar cartão, continua com a memória RAM e as sessões no Safari. Se encontrar cartão, inicia automaticamente uma nova sessão CSV, salvo se definires `ODL_SD_AUTOSTART 0` no início do sketch. Podes também parar/iniciar a gravação a partir de `Sessões → microSD`. A deteção só ocorre no arranque; se ligares o cartão com a placa já ligada, reinicia-a.

Os ficheiros `ODL0000.CSV`, `ODL0001.CSV`… contêm comentários de metadados (`#`), amostras `S;` e flancos `E;`, para descarregar ou importar novamente na Web. Esta função ainda precisa de testes com cartão real; atrasos de escrita podem encher a FIFO e provocar perdas. Um corte de energia pode deixar o último ficheiro incompleto.

A escrita é agrupada em RAM, fora das interrupções; em operações SD muito lentas a FIFO pode encher. Testar cartões, desligamento inesperado, capacidade e integridade dos ficheiros no hardware real. O formato ainda não suporta recuperação transaccional após falha de alimentação nem rotação automática de ficheiros grandes.

## Limitações importantes

- Na UNO R4, seis entradas analógicas são multiplexadas: um varrimento não é estritamente simultâneo. Os estados digitais têm relógio comum mas tempos de IRQ aproximados, sujeitos a latência e disponibilidade de memória.
- O ring buffer ADC tem reserva **fixa** de 4096 bytes. Com quatro canais activos ocupa 12 bytes/registo ⇒ 341 registos (~34 s a 100 ms). O histórico de eventos tem 128 flancos. Não há expansão de RAM automática.
- Os cursores usam a leitura mais próxima no histórico para os Δvalores; os cálculos de ciclos contabilizam apenas flancos completos presentes na fonte. Não significam tempos totais de um processo parcialmente registado.
- Canais calculados não são novas entradas físicas e são calculados pela Web. Comparações de experiências longas podem usar subamostragem apenas no **desenho**, sem modificar as amostras exportadas.
- A gravação em cartão ainda não foi ensaiada; a configuração Wi-Fi via HTTP e os limites de EEPROM também requerem ensaio físico.
- **UNO Q, expansão de ADC/I²C, drivers de equipamentos concretos e interface segura de 230 V não estão implementados**: exigem requisitos de hardware e validação próprios. Esta versão preserva uma arquitectura que pode ser portada, não é ainda uma compilação para UNO Q.

Não ligar tensão de rede de 230 V aos pinos Arduino. Para o compressor e a válvula do frigorífico, usar isolamento eléctrico apropriado, por exemplo entradas isoladas certificadas, validadas por um técnico competente.

Ver `TEST_PLAN_1.0.2.md`, `CHANGELOG_1.0.2.md` e `PROTOCOL_ODLB_V2.md` para mais pormenores.


## Novidades 1.0.2
Ver `CHANGELOG_1.0.2.md` e o perfil `ChannelDefaults.h`.

**Modo de funcionamento:** a placa arranca em AQUISIÇÃO REAL; a página Web permite mudar para DEMO para testar.
**NTC:** colocar o canal no tipo NTC, passar para REAL, aplicar temperatura de referência estabilizada e guardar os pontos em Análise.
**Sessões:** Iniciar/Parar afectam só a gravação do Safari, não a aquisição do Arduino.
**microSD:** FAT16/FAT32, aconselhado até 32 GB; formatar no computador, não na Web.
