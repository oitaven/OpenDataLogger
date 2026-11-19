# Migração da versão 1.0.0 para 1.0.1

1. Guardar uma cópia da pasta actual `OpenDataLogger`.
2. Copiar o novo conjunto de ficheiros (`OpenDataLogger.ino`, `WebPage.ino`, `AdvancedFeatures.h`, `OptionalSD.h` e os restantes documentos), **sem substituir o teu `arduino_secrets.h`**, que contém a configuração pessoal.
3. A compilação deixa de exigir a variável `WEB_ADMIN_KEY`. Para poderes alterar o Wi-Fi na Web, acrescentar ao `arduino_secrets.h` uma linha como a seguinte, com uma chave diferente e confidencial:

   ```cpp
   #define ODL_WEB_ADMIN_KEY "SUBSTITUIR_POR_UMA_CHAVE_FORTE_E_PESSOAL"
   ```

   Se esta macro não existir, o programa compila e o formulário Web de redes permanece desactivado. O antigo `const char* WEB_ADMIN_KEY` pode ser eliminado.
4. A partir desta versão, o microSD é detectado no arranque. Mesmo sem leitor/cartão, a aquisição continua pela RAM e pelo Safari. Quando ligares um cartão compatível, reinicia a placa: a gravação CSV inicia automaticamente. O comportamento pode ser alterado com `ODL_SD_AUTOSTART 0` no sketch.
5. As configurações guardadas em EEPROM permanecem. Para substituir os nomes antigos por `A0`–`A5`, `D2` e `D3`, usar `Canais → Repor nomes genéricos`, sair do separador e clicar em `Guardar configuração`.
6. Na primeira execução, verificar em `Sistema` as perdas FIFO e a temporização da aquisição. Para o microSD, confirmar ausência de erros e não desligar durante uma gravação sem parar primeiro.

**Atenção:** com a compilação SD activada, é necessária a biblioteca Arduino `SD` (e `SPI`) instalada no Arduino IDE. A inicialização e as escritas no cartão ainda não foram ensaiadas nesta placa por esta revisão.
