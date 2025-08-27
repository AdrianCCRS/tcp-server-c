# Servidor TCP en C con Thread Pool y Pruebas de Carga con JMeter

Este proyecto implementa un **servidor TCP en C** diseñado para manejar alta concurrencia utilizando un **pool de hilos (thread pool)**.  
El objetivo es lograr **bajo tiempo de respuesta**, **alta eficiencia** y **robustez** en escenarios con miles de conexiones concurrentes.

Un servidor TCP multi-hilo que cuenta las ocurrencias del último carácter en cadenas de texto que contengan solo caracteres del alfabeto.

---

## 🛠️ Arquitectura del Servidor

### Diseño Técnico
- **Lenguaje:** C (sockets TCP nativos)
- **Concurrencia:** Pool de hilos con número configurable de hilos (por defecto 200 para alta carga)
- **Sincronización:** Cola de clientes protegida con `mutex` y variables de condición
- **Gestión de conexiones:** Aceptación de múltiples clientes con `listen()` y cierre controlado de sockets
- **Manejo de señales:** Apagado seguro mediante `signalfd`
- **Estadísticas en tiempo real:** Conteo de clientes procesados y activos

### Características Funcionales
- **Servidor TCP Multi-hilo**: Utiliza un pool de hilos para manejar múltiples clientes concurrentemente
- **Validación de entrada**: Solo acepta mensajes que contengan caracteres del alfabeto (a-z, A-Z)
- **Conteo de caracteres**: Cuenta cuántas veces aparece el último carácter en la cadena de texto
- **Puerto configurable**: Puerto por defecto 5050, configurable via parámetros
- **Gestión de señales**: Manejo graceful de SIGINT y SIGTERM para cierre limpio
- **Gestión de memoria**: Sin memory leaks, cleanup completo de recursos

---

## 🚀 Estrategias de Diseño

### Optimizaciones de Rendimiento
- **Escalabilidad mediante pool de hilos**: Número configurable optimizado para alta carga
- **Backlog ampliado**: `listen()` configurado para 1024 conexiones simultáneas
- **Cola optimizada**: Queue size = threads × 20 para manejar picos de carga
- **Validaciones eficientes**: Validación de entrada rápida con buffers protegidos
- **Cierre controlado**: Cierre seguro de conexiones tras cada petición

### Componentes del Sistema

#### Servidor (`server/server.c`)
- **Thread Pool**: Pool configurable de hilos (200 por defecto para producción)
- **Cola de clientes**: Implementa una cola thread-safe para manejar clientes pendientes
- **Sincronización**: Utiliza mutexes y variables de condición para thread safety
- **Signal handling**: Usa signalfd para manejo asíncrono de señales
- **Polling**: Usa poll() para manejar conexiones entrantes y señales
- **Tracking**: Monitoreo en tiempo real de clientes procesados y activos

#### Cliente (`client/vowel_client.c`)
- **Validación local**: Verifica que el texto contenga solo caracteres del alfabeto
- **Protocolo simple**: Envía texto terminado en newline
- **Respuesta única**: Recibe y muestra la respuesta del servidor

---

## 📊 Resultados de las Pruebas (JMeter)

Las pruebas de carga se realizaron con **Apache JMeter**:

### Configuración de Prueba
- **Usuarios concurrentes:** 10,000  
- **Ramp-up:** 2 segundos  
- **Iteraciones por usuario:** 10  
- **Total de peticiones:** 100,000  

### Resultados Obtenidos

| Métrica                     | Valor                          |
|------------------------------|--------------------------------|
| # Samples                    | 100,000                        |
| Tiempo promedio de respuesta | ~2 ms                          |
| Throughput                   | ~4,991 peticiones/segundo      |
| Error %                      | 0%                             |
| Latencia                     | 0 - 2 ms                       |

---

## 💡 Razones para Usar C vs Java/Python

### Ventajas del Servidor en C
- **Control total** sobre recursos del sistema y gestión de memoria
- **Máximo rendimiento** sin overhead de máquinas virtuales o interpretes
- **Escalabilidad real** con técnicas de concurrencia eficientes a nivel de OS
- **Bajo consumo** de CPU y RAM frente a lenguajes de alto nivel
- **Latencia mínima** sin garbage collection o pausas impredecibles
- **Portabilidad** en sistemas Unix/Linux sin dependencias externas

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
# Configuración por defecto (puerto 5050, 200 hilos)
./run_server.sh

# Configuración personalizada
./run_server.sh <ip> <puerto> <num_hilos>

# Ejemplos:
./run_server.sh 127.0.0.1 5050 500    # Para alta carga (1M+ clientes)
./run_server.sh 127.0.0.1 8080 100    # Para carga media
./run_server.sh 127.0.0.1 5050 50     # Para pruebas locales

# Ejecución directa
./build/server/server_app <ip> <puerto> <num_hilos>
```

### Configuración Recomendada de Hilos

| Escenario | Hilos | Descripción |
|-----------|-------|-------------|
| **Desarrollo/Test** | 8-16 | CPU cores × 2-4 |
| **Producción Media** | 50-100 | Para miles de clientes |
| **Producción Alta** | 200-500 | Para cientos de miles |
| **Producción Extrema** | 500-1000 | Para millones de clientes |

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

1. **Cliente → Servidor**: Envía texto terminado en `\n`
2. **Servidor**: 
   - Valida que solo contenga a-z, A-Z
   - Si es válido: cuenta ocurrencias del último carácter
   - Si es inválido: envía mensaje de error
3. **Servidor → Cliente**: Envía respuesta y cierra conexión

## Gestión de hilos

- **Thread Pool**: Se crean el número de hilos seteados
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

---
