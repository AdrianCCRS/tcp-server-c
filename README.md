# TCP Character Counter Server

Un servidor TCP multi-hilo que cuenta las ocurrencias del último carácter en cadenas de texto que contengan solo caracteres del alfabeto.

## Características

- **Servidor TCP Multi-hilo**: Utiliza un pool de hilos para manejar múltiples clientes concurrentemente
- **Validación de entrada**: Solo acepta mensajes que contengan caracteres del alfabeto (a-z, A-Z)
- **Conteo de caracteres**: Cuenta cuántas veces aparece el último carácter en la cadena de texto
- **Puerto configurable**: Puerto por defecto 5050, configurable via parámetros
- **Gestión de señales**: Manejo graceful de SIGINT y SIGTERM para cierre limpio
- **Gestión de memoria**: Sin memory leaks, cleanup completo de recursos

## Arquitectura

### Servidor (`server/server.c`)
- **Thread Pool**: Crea un pool de hilos basado en el número de CPUs disponibles
- **Cola de clientes**: Implementa una cola thread-safe para manejar clientes pendientes
- **Sincronización**: Utiliza mutexes y variables de condición para thread safety
- **Signal handling**: Usa signalfd para manejo asíncrono de señales
- **Polling**: Usa poll() para manejar conexiones entrantes y señales

### Cliente (`client/vowel_client.c`)
- **Validación local**: Verifica que el texto contenga solo caracteres del alfabeto
- **Protocolo simple**: Envía texto terminado en newline
- **Respuesta única**: Recibe y muestra la respuesta del servidor

## Compilación

### Opción 1: Script automático
```bash
chmod +x build.sh
./build.sh
```

### Opción 2: Compilación manual
```bash
# Servidor
gcc -o build/server/server_app server/server.c -lpthread

# Cliente  
gcc -o build/client/vowel_client client/vowel_client.c
```

## Uso

### Iniciar el servidor
```bash
# Puerto por defecto (5050)
./run_server.sh

# Puerto personalizado
./build/server/server_app 127.0.0.1 8080
```

### Usar el cliente
```bash
# Sintaxis
./build/client/vowel_client <server_ip> <server_port> <text>

# Ejemplos válidos
./build/client/vowel_client 127.0.0.1 5050 hello
./build/client/vowel_client 127.0.0.1 5050 programming
./build/client/vowel_client 127.0.0.1 5050 aabbcc

# Ejemplos inválidos (serán rechazados)
./build/client/vowel_client 127.0.0.1 5050 hello123  # contiene números
./build/client/vowel_client 127.0.0.1 5050 "hello world"  # contiene espacio
```

## Ejemplos de respuesta

### Entrada válida:
```bash
$ ./build/client/vowel_client 127.0.0.1 5050 programming
Enviando mensaje: 'programming'
Respuesta del servidor:
Último carácter 'g' encontrado 2 veces
```

### Entrada inválida:
```bash
$ ./build/client/vowel_client 127.0.0.1 5050 hello123
Error: Text must contain only alphabet characters (a-z, A-Z)
```

## Protocolo de comunicación

1. **Cliente → Servidor**: Envía texto terminado en `
`
2. **Servidor**: 
   - Valida que solo contenga a-z, A-Z
   - Si es válido: cuenta ocurrencias del último carácter
   - Si es inválido: envía mensaje de error
3. **Servidor → Cliente**: Envía respuesta y cierra conexión

## Gestión de hilos

- **Thread Pool**: Se crea un hilo por CPU disponible
- **Worker threads**: Procesan clientes de forma independiente
- **Thread safety**: Cola protegida por mutex y condition variables
- **Graceful shutdown**: Los hilos terminan limpiamente al recibir señales

## Gestión de memoria

- **Sin memory leaks**: Todos los recursos se liberan correctamente
- **Client sockets**: Se cierran y liberan después del procesamiento
- **Thread pool**: Se destruye completamente al finalizar
- **Synchronization objects**: Mutexes y condition variables se destruyen

## Requisitos del sistema

- **OS**: Linux (usa signalfd, poll)
- **Compilador**: GCC con soporte para pthreads
- **Bibliotecas**: pthread, estándar C

## Arquitectura de archivos

```
tcp/
├── README.md
├── build.sh                 # Script de compilación
├── run_server.sh           # Script para ejecutar servidor
├── server/
│   └── server.c            # Código del servidor
├── client/
│   └── vowel_client.c      # Código del cliente
└── build/
    ├── server/
    │   └── server_app      # Ejecutable del servidor
    └── client/
        └── vowel_client    # Ejecutable del cliente
```

## Desarrollo

### Proceso implementado

1. **Diseño inicial**: Servidor TCP básico con threads
2. **Thread Pool**: Implementación de pool de hilos con cola thread-safe
3. **Protocolo**: Definición de protocolo simple (texto + newline)
4. **Validación**: Implementación de validación de caracteres alfabéticos
5. **Signal handling**: Manejo graceful de señales para cierre limpio
6. **Memory management**: Eliminación de memory leaks
7. **Optimización**: Uso de signalfd y poll para mejor rendimiento
8. **Testing**: Pruebas con diferentes tipos de entrada

### Características técnicas

- **Concurrencia**: Pool de hilos para manejar múltiples clientes
- **Escalabilidad**: Número de hilos basado en CPUs disponibles
- **Robustez**: Manejo de errores y cleanup de recursos
- **Eficiencia**: Uso de polling en lugar de blocking calls
- **Portabilidad**: Código compatible con sistemas Linux modernos

## Requisitos

- **Java 8+** (para el servidor)
- **GCC** (para el cliente en C)
- **Linux/macOS** (probado en sistemas Unix)

## Instalación y Compilación

### 1. Compilar todo el proyecto
```bash
./build.sh
```

Este script:
- Compila el servidor Java en `build/server/`
- Compila el cliente C en `build/client/`

## Uso

### 1. Ejecutar el servidor
```bash
./run_server.sh
```

El servidor:
- Escucha en `127.0.0.1:9595` por defecto
- Mantiene funcionando hasta presionar **Ctrl+C**
- Acepta múltiples clientes consecutivamente

### 2. Ejecutar el cliente
```bash
./run_client.sh "texto a procesar" 4 2
```

Parámetros:
- **texto**: Texto a analizar
- **hilos**: Número de hilos a usar
- **chunks**: Número de fragmentos para dividir el texto

### Ejemplo completo:
```bash
# Terminal 1: Iniciar servidor
./run_server.sh

# Terminal 2: Enviar solicitud
./run_client.sh "hello world example" 3 2
```

## Funcionamiento

1. **Cliente** envía: `texto|hilos|chunks`
2. **Servidor** divide el texto en fragmentos
3. **Hilos** procesan cada fragmento en paralelo
4. **Servidor** envía resultados en tiempo real
5. **Cliente** muestra los resultados

## Salida de Ejemplo

```
Recibiendo respuestas del servidor:
Proceso iniciado - enviando resultados en tiempo real:
Fragmento 0 (tamaño: 8): hello wo
Fragmento 1 (tamaño: 9): rld example

Tarea 0 ejecutada por HILO[pool-1-thread-1]
Tarea 0 - Contando 'e' en el fragmento: hello wo
Tarea 0 encontró 1 ocurrencias.
----------------------------------------

Tarea 1 ejecutada por HILO[pool-1-thread-2]
Tarea 1 - Contando 'e' en el fragmento: rld example
Tarea 1 encontró 2 ocurrencias.
----------------------------------------

PROCESS_COMPLETED
Proceso completado.
```

## Configuración Personalizada

### Cambiar IP y puerto del servidor:
```bash
java -cp ./build/server server.Server 192.168.1.100 8080
```

### Ejecutar cliente con IP personalizada:
```bash
./build/client/vowel_client 192.168.1.100 8080 "texto" 4 2
```

## Solución de Problemas

### Error: "Could not find or load main class Server"
```bash
# Recompilar
./build.sh
```

### Error: "Connection refused"
- Verificar que el servidor esté ejecutándose
- Comprobar IP y puerto correctos

### Error de compilación
- Verificar que Java y GCC estén instalados
- Verificar permisos de ejecución: `chmod +x *.sh`

## Detener el Servidor

Presiona **Ctrl+C** en el terminal del servidor para detenerlo de forma segura.
