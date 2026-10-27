
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#define MAX 1024
#define MAX_TOKENS 512
#define MAX_TOKEN 64
#define MAX_RESULTADO 4096

#define OPERANDO 1
#define OPERADOR 2
#define PARENTESIS_ABRE 3
#define PARENTESIS_CIERRA 4

// Guarda cada elemento de la expresion.
typedef struct {
    char texto[MAX_TOKEN];
    int tipo;
} Token;

// Pila de operadores.
Token Pila[MAX_TOKENS];
int Tope = 0;

// Prioridad de operadores.
int prioridad(char c) {
    if (c == '^')
        return 3;
    else if (c == '*' || c == '/')
        return 2;
    else if (c == '+' || c == '-')
        return 1;
    else
        return 0;
}

// Verifica si es operador.
int es_operador(char c) {
    return c == '+' || c == '-' ||
           c == '*' || c == '/' || c == '^';
}

// Separa los elementos de la expresion.
int analizar_expresion(const char expresion[],
                       Token tokens[], int *cantidad) {
    int i = 0;
    int n = 0;

    while (expresion[i] != '\0') {
        unsigned char c = (unsigned char) expresion[i];

        // Ignorar espacios.
        if (isspace(c)) {
            i++;
            continue;
        }

        if (n >= MAX_TOKENS) {
            printf("Error: demasiados elementos en la expresion.\n");
            return 0;
        }

        // Leer parentesis.
        if (expresion[i] == '(' || expresion[i] == ')') {
            tokens[n].texto[0] = expresion[i];
            tokens[n].texto[1] = '\0';

            if (expresion[i] == '(')
                tokens[n].tipo = PARENTESIS_ABRE;
            else
                tokens[n].tipo = PARENTESIS_CIERRA;

            n++;
            i++;
        }

        // Leer operadores.
        else if (es_operador(expresion[i])) {
            tokens[n].texto[0] = expresion[i];
            tokens[n].texto[1] = '\0';
            tokens[n].tipo = OPERADOR;

            n++;
            i++;
        }

        // Leer numeros y variables.
        else if (isalnum(c) || expresion[i] == '_' ||
                 (expresion[i] == '.' &&
                  isdigit((unsigned char) expresion[i + 1]))) {
            int j = 0;
            int tiene_punto = 0;

            while (isalnum((unsigned char) expresion[i]) ||
                   expresion[i] == '_' ||
                   (expresion[i] == '.' &&
                    isdigit((unsigned char) expresion[i + 1]))) {

                // Validar decimal.
                if (expresion[i] == '.') {
                    if (tiene_punto) {
                        printf("Error: numero decimal incorrecto.\n");
                        return 0;
                    }
                    tiene_punto = 1;
                }

                // Revisar longitud.
                if (j >= MAX_TOKEN - 1) {
                    printf("Error: operando demasiado largo.\n");
                    return 0;
                }

                tokens[n].texto[j++] = expresion[i++];
            }

            tokens[n].texto[j] = '\0';
            tokens[n].tipo = OPERANDO;
            n++;
        }

        // Caracter no valido.
        else {
            printf("Error: caracter no permitido: %c\n",
                   expresion[i]);
            return 0;
        }
    }

    *cantidad = n;
    return 1;
}

// Convierte de infija a prefija.
int CONV_PREFIJA(Token entrada[], int cantidad,
                 char ExpPRE[]) {
    Token salida[MAX_TOKENS];
    int n_salida = 0;
    int i;

    Tope = 0;

    // Recorrer de derecha a izquierda.
    for (i = cantidad - 1; i >= 0; i--) {

        // Pasar operandos.
        if (entrada[i].tipo == OPERANDO) {
            salida[n_salida++] = entrada[i];
        }

        // Guardar parentesis derecho.
        else if (entrada[i].tipo == PARENTESIS_CIERRA) {
            Pila[Tope++] = entrada[i];
        }

        // Resolver parentesis.
        else if (entrada[i].tipo == PARENTESIS_ABRE) {
            while (Tope > 0 &&
                   Pila[Tope - 1].tipo != PARENTESIS_CIERRA) {
                salida[n_salida++] = Pila[--Tope];
            }

            if (Tope == 0) {
                printf("Error: parentesis desbalanceados.\n");
                return 0;
            }

            Tope--;
        }

        // Ordenar operadores.
        else if (entrada[i].tipo == OPERADOR) {
            while (Tope > 0 &&
                   Pila[Tope - 1].tipo == OPERADOR &&
                   (prioridad(Pila[Tope - 1].texto[0]) >
                    prioridad(entrada[i].texto[0]) ||
                    (prioridad(Pila[Tope - 1].texto[0]) ==
                     prioridad(entrada[i].texto[0]) &&
                     entrada[i].texto[0] == '^'))) {

                salida[n_salida++] = Pila[--Tope];
            }

            Pila[Tope++] = entrada[i];
        }
    }

    // Vaciar la pila.
    while (Tope > 0) {
        if (Pila[Tope - 1].tipo != OPERADOR) {
            printf("Error: parentesis desbalanceados.\n");
            return 0;
        }

        salida[n_salida++] = Pila[--Tope];
    }

    // Formar la expresion prefija.
    ExpPRE[0] = '\0';

    for (i = n_salida - 1; i >= 0; i--) {
        size_t longitud = strlen(ExpPRE);
        size_t longitud_token = strlen(salida[i].texto);
        size_t espacio = (longitud > 0) ? 1 : 0;

        if (longitud + espacio + longitud_token >= MAX_RESULTADO) {
            printf("Error: resultado demasiado largo.\n");
            return 0;
        }

        if (espacio)
            strcat(ExpPRE, " ");

        strcat(ExpPRE, salida[i].texto);
    }

    return 1;
}

// Funcion principal.
int main(void) {
    FILE *archivo;
    char nombre_archivo[MAX];
    char ExpINF[MAX];
    char ExpPRE[MAX_RESULTADO];
    Token tokens[MAX_TOKENS];
    int cantidad;
    int numero_linea = 0;

    // Pedir el archivo.
    printf("Dame el nombre del archivo que deseas leer: ");

    if (fgets(nombre_archivo, sizeof(nombre_archivo), stdin) == NULL)
        return 1;

    nombre_archivo[strcspn(nombre_archivo, "\r\n")] = '\0';

    if (nombre_archivo[0] == '\0') {
        printf("Error: no se introdujo un nombre de archivo.\n");
        return 1;
    }

    // Abrir archivo.
    archivo = fopen(nombre_archivo, "r");

    if (archivo == NULL) {
        printf("Error: no se pudo abrir el archivo '%s'.\n",
               nombre_archivo);
        printf("Verifica el nombre y la ubicacion del archivo.\n");
        return 1;
    }

    printf("\nCONVERSION DE INFIJA A PREFIJA\n");
    printf("Archivo: %s\n", nombre_archivo);
    printf("====================================\n");

    // Leer expresiones del archivo.
    while (fgets(ExpINF, sizeof(ExpINF), archivo) != NULL) {
        numero_linea++;

        ExpINF[strcspn(ExpINF, "\r\n")] = '\0';

        // Saltar lineas vacias.
        if (ExpINF[strspn(ExpINF, " \t\r\n")] == '\0')
            continue;

        printf("\nLinea %d\n", numero_linea);
        printf("Infija:  %s\n", ExpINF);

        if (!analizar_expresion(ExpINF, tokens, &cantidad))
            continue;

        if (cantidad == 0) {
            printf("Error: expresion vacia.\n");
            continue;
        }

        // Mostrar la conversion.
        if (CONV_PREFIJA(tokens, cantidad, ExpPRE)) {
            printf("Prefija: %s\n", ExpPRE);
        } else {
            printf("No se pudo realizar la conversion.\n");
        }
    }

    // Cerrar archivo.
    fclose(archivo);

    printf("\n====================================\n");
    printf("Lectura del archivo finalizada.\n");

    return 0;
}


