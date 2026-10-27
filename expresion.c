
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#define MAX_NOMBRE 256
#define MAX_LINEA 4096
#define MAX_TOKEN 256

/* Guarda si un comentario de bloque sigue abierto entre lineas. */
static int en_comentario_bloque = 0;

/* Un identificador comienza con una letra o guion bajo. */
int es_inicio_identificador(char c) {
    return isalpha((unsigned char)c) || c == '_';
}

/* Despues puede contener letras, numeros o guion bajo. */
int es_parte_identificador(char c) {
    return isalnum((unsigned char)c) || c == '_';
}

/* Compara el token con los tipos de datos comunes de C. */
int es_tipo_dato(const char *token) {
    const char *tipos[] = {
        "void", "char", "short", "int", "long", "float", "double",
        "signed", "unsigned", "_Bool", "_Complex", "_Imaginary",
        "size_t", NULL
    };
    int i;

    for (i = 0; tipos[i] != NULL; i++) {
        if (strcmp(token, tipos[i]) == 0)
            return 1;
    }

    return 0;
}

/* Reconoce palabras usadas en condiciones, ciclos y saltos. */
int es_instruccion(const char *token) {
    const char *palabras[] = {
        "if", "else", "switch", "case", "default", "while", "do",
        "for", "break", "continue", "return", "goto", NULL
    };
    int i;

    for (i = 0; palabras[i] != NULL; i++) {
        if (strcmp(token, palabras[i]) == 0)
            return 1;
    }

    return 0;
}

/* Revisa otras palabras reservadas que no son tipos ni instrucciones. */
int es_palabra_reservada(const char *token) {
    const char *palabras[] = {
        "auto", "const", "enum", "extern", "inline", "register",
        "restrict", "static", "struct", "typedef", "union", "volatile",
        "_Atomic", "_Generic", "_Noreturn", "_Static_assert",
        "_Thread_local", "sizeof", "typeof", "alignas", "alignof",
        "asm", "bool", NULL
    };
    int i;

    for (i = 0; palabras[i] != NULL; i++) {
        if (strcmp(token, palabras[i]) == 0)
            return 1;
    }

    return 0;
}

/* Reconoce palabras que pueden aparecer despues del simbolo #. */
int es_directiva(const char *token) {
    const char *directivas[] = {
        "include", "define", "undef", "if", "ifdef", "ifndef",
        "elif", "else", "endif", "error", "pragma", "line", NULL
    };
    int i;

    for (i = 0; directivas[i] != NULL; i++) {
        if (strcmp(token, directivas[i]) == 0)
            return 1;
    }

    return 0;
}

/* Valida enteros y decimales con exponentes o sufijos comunes. */
int es_numero(const char *token) {
    int i = 0;
    int digitos = 0;
    int punto = 0;
    int exponente = 0;

    if (token[0] == '\0')
        return 0;

    while (token[i] != '\0') {
        if (isdigit((unsigned char)token[i])) {
            digitos++;
        } else if (token[i] == '.' && !punto && !exponente) {
            punto = 1;
        } else if ((token[i] == 'e' || token[i] == 'E') &&
                   !exponente && digitos > 0) {
            exponente = 1;

            if (token[i + 1] == '+' || token[i + 1] == '-')
                i++;

            if (!isdigit((unsigned char)token[i + 1]))
                return 0;

            digitos = 0;
        } else if ((token[i] == 'f' || token[i] == 'F' ||
                    token[i] == 'u' || token[i] == 'U' ||
                    token[i] == 'l' || token[i] == 'L') &&
                   digitos > 0) {
            while (token[i] != '\0') {
                if (token[i] != 'f' && token[i] != 'F' &&
                    token[i] != 'u' && token[i] != 'U' &&
                    token[i] != 'l' && token[i] != 'L')
                    return 0;

                i++;
            }

            return 1;
        } else {
            return 0;
        }

        i++;
    }

    return digitos > 0;
}

/* Muestra el token, su categoria y la linea donde se encontro. */
void mostrar_token(const char *token, const char *categoria, int linea) {
    printf("Linea %d: %-24s -> %s\n", linea, token, categoria);
}

/* Decide si una palabra es tipo, instruccion, reservada o variable. */
void clasificar_identificador(const char *token, int linea,
                              int despues_de_numero) {
    if (despues_de_numero) {
        mostrar_token(token, "SUFIJO/IDENTIFICADOR", linea);
    } else if (es_tipo_dato(token)) {
        mostrar_token(token, "TIPO DE DATO", linea);
    } else if (es_instruccion(token)) {
        mostrar_token(token, "INSTRUCCION DE CONTROL", linea);
    } else if (es_directiva(token)) {
        mostrar_token(token, "DIRECTIVA", linea);
    } else if (es_palabra_reservada(token)) {
        mostrar_token(token, "PALABRA RESERVADA", linea);
    } else if (strcmp(token, "true") == 0 ||
               strcmp(token, "false") == 0 ||
               strcmp(token, "NULL") == 0) {
        mostrar_token(token, "CONSTANTE", linea);
    } else {
        mostrar_token(token, "IDENTIFICADOR/VARIABLE", linea);
    }
}

/* Indica si el caracter puede iniciar un operador. */
int es_operador_inicio(char c) {
    return strchr("+-*/%=<>!&|^~?:", c) != NULL;
}

/* Estos signos delimitan grupos, argumentos o instrucciones. */
int es_separador(char c) {
    return strchr("(){}[],;", c) != NULL;
}

/* Busca primero operadores largos, como ==, ++ o >>=. */
int longitud_operador(const char *texto) {
    static const char *operadores[] = {
        ">>=", "<<=", "...", "++", "--", "->", "&&", "||", "==", "!=",
        "<=", ">=", "+=", "-=", "*=", "/=", "%=", "&=", "|=", "^=",
        "<<", ">>", NULL
    };
    int i;

    for (i = 0; operadores[i] != NULL; i++) {
        size_t n = strlen(operadores[i]);

        if (strncmp(texto, operadores[i], n) == 0)
            return (int)n;
    }

    return 1;
}

/* Agrupa los operadores por su funcion para mostrarlos claramente. */
void clasificar_operador(const char *token, int linea) {
    if (strcmp(token, "=") == 0 || strcmp(token, "+=") == 0 ||
        strcmp(token, "-=") == 0 || strcmp(token, "*=") == 0 ||
        strcmp(token, "/=") == 0 || strcmp(token, "%=") == 0 ||
        strcmp(token, "&=") == 0 || strcmp(token, "|=") == 0 ||
        strcmp(token, "^=") == 0 || strcmp(token, "<<=") == 0 ||
        strcmp(token, ">>=") == 0) {
        mostrar_token(token, "OPERADOR DE ASIGNACION", linea);
    } else if (strcmp(token, "==") == 0 || strcmp(token, "!=") == 0 ||
               strcmp(token, "<") == 0 || strcmp(token, ">") == 0 ||
               strcmp(token, "<=") == 0 || strcmp(token, ">=") == 0) {
        mostrar_token(token, "OPERADOR RELACIONAL", linea);
    } else if (strcmp(token, "&&") == 0 || strcmp(token, "||") == 0 ||
               strcmp(token, "!") == 0) {
        mostrar_token(token, "OPERADOR LOGICO", linea);
    } else if (strcmp(token, "&") == 0 || strcmp(token, "|") == 0 ||
               strcmp(token, "^") == 0 || strcmp(token, "~") == 0 ||
               strcmp(token, "<<") == 0 || strcmp(token, ">>") == 0) {
        mostrar_token(token, "OPERADOR BIT A BIT", linea);
    } else if (strcmp(token, "++") == 0 || strcmp(token, "--") == 0) {
        mostrar_token(token, "OPERADOR DE INCREMENTO/DECREMENTO", linea);
    } else if (strcmp(token, "->") == 0) {
        mostrar_token(token, "OPERADOR DE ACCESO A MIEMBRO", linea);
    } else if (strcmp(token, "?") == 0 || strcmp(token, ":") == 0) {
        mostrar_token(token, "OPERADOR CONDICIONAL", linea);
    } else {
        mostrar_token(token, "OPERADOR ARITMETICO", linea);
    }
}

/* Recorre la linea caracter por caracter, aunque no haya espacios. */
void analizar_linea(const char *linea_texto, int numero_linea) {
    size_t i = 0;
    size_t longitud = strlen(linea_texto);

    while (i < longitud) {
        char token[MAX_TOKEN];
        size_t j = 0;

        /* Si el comentario empezo antes, busca donde termina. */
        if (en_comentario_bloque) {
            while (i + 1 < longitud &&
                   !(linea_texto[i] == '*' && linea_texto[i + 1] == '/'))
                i++;

            if (i + 1 < longitud) {
                en_comentario_bloque = 0;
                i += 2;
            } else {
                return;
            }

            continue;
        }

        /* Los espacios se ignoran; no se imprimen como tokens. */
        if (isspace((unsigned char)linea_texto[i])) {
            i++;
            continue;
        }

        /* Un comentario // hace que se ignore el resto de la linea. */
        if (i + 1 < longitud && linea_texto[i] == '/' &&
            linea_texto[i + 1] == '/')
            return;

        /* Un comentario de bloque puede continuar en otra linea. */
        if (i + 1 < longitud && linea_texto[i] == '/' &&
            linea_texto[i + 1] == '*') {
            en_comentario_bloque = 1;
            i += 2;
            continue;
        }

        /* Lee una palabra completa: variable, tipo o palabra reservada. */
        if (es_inicio_identificador(linea_texto[i])) {
            while (i < longitud &&
                   es_parte_identificador(linea_texto[i])) {
                if (j + 1 < sizeof(token))
                    token[j++] = linea_texto[i];

                i++;
            }

            token[j] = '\0';
            clasificar_identificador(token, numero_linea, 0);
            continue;
        }

        /* Junta el numero completo y despues comprueba su formato. */
        if (isdigit((unsigned char)linea_texto[i]) ||
            (linea_texto[i] == '.' && i + 1 < longitud &&
             isdigit((unsigned char)linea_texto[i + 1]))) {
            int punto = 0;
            int exponente = 0;

            while (i < longitud) {
                char c = linea_texto[i];

                if (isalnum((unsigned char)c) || c == '_') {
                    if (j + 1 < sizeof(token))
                        token[j++] = c;

                    if (c == 'e' || c == 'E')
                        exponente = 1;

                    i++;
                } else if (c == '.') {
                    if (j + 1 < sizeof(token))
                        token[j++] = c;

                    if (punto && !exponente) {
                        i++;

                        while (i < longitud &&
                               (isalnum((unsigned char)linea_texto[i]) ||
                                linea_texto[i] == '.')) {
                            if (j + 1 < sizeof(token))
                                token[j++] = linea_texto[i];

                            i++;
                        }

                        break;
                    }

                    punto = 1;
                    i++;
                } else if ((c == '+' || c == '-') && j > 0 &&
                           (token[j - 1] == 'e' ||
                            token[j - 1] == 'E')) {
                    if (j + 1 < sizeof(token))
                        token[j++] = c;

                    i++;
                } else {
                    break;
                }
            }

            token[j] = '\0';

            if (es_numero(token))
                mostrar_token(token, "NUMERO", numero_linea);
            else
                mostrar_token(token, "NUMERO NO VALIDO", numero_linea);

            continue;
        }

        /* Las comillas mantienen unido el texto, incluso con espacios. */
        if (linea_texto[i] == '"' || linea_texto[i] == '\'') {
            char comilla = linea_texto[i];
            int cerrada = 0;

            if (j + 1 < sizeof(token))
                token[j++] = linea_texto[i];

            i++;

            while (i < longitud) {
                char c = linea_texto[i];

                if (j + 1 < sizeof(token))
                    token[j++] = c;

                i++;

                if (c == '\\' && i < longitud) {
                    if (j + 1 < sizeof(token))
                        token[j++] = linea_texto[i];

                    i++;
                } else if (c == comilla) {
                    cerrada = 1;
                    break;
                }
            }

            token[j] = '\0';

            if (!cerrada)
                mostrar_token(token, "CADENA/LITERAL SIN CERRAR", numero_linea);
            else if (comilla == '"')
                mostrar_token(token, "CADENA DE TEXTO", numero_linea);
            else
                mostrar_token(token, "CARACTER", numero_linea);

            continue;
        }

        /* Reconoce operadores largos antes de los operadores simples. */
        if (es_operador_inicio(linea_texto[i])) {
            int n = longitud_operador(&linea_texto[i]);
            int k;

            for (k = 0; k < n && j + 1 < sizeof(token); k++)
                token[j++] = linea_texto[i + (size_t)k];

            token[j] = '\0';
            clasificar_operador(token, numero_linea);
            i += (size_t)n;
            continue;
        }

        /* Clasifica parentesis, llaves, corchetes y separadores. */
        if (es_separador(linea_texto[i])) {
            token[0] = linea_texto[i];
            token[1] = '\0';

            if (strchr("()", linea_texto[i]) != NULL)
                mostrar_token(token, "PARENTESIS", numero_linea);
            else if (strchr("{}", linea_texto[i]) != NULL)
                mostrar_token(token, "LLAVE", numero_linea);
            else if (strchr("[]", linea_texto[i]) != NULL)
                mostrar_token(token, "CORCHETE", numero_linea);
            else
                mostrar_token(token, "SEPARADOR", numero_linea);

            i++;
            continue;
        }

        /* El punto tambien se usa para acceder a miembros de estructuras. */
        if (linea_texto[i] == '.') {
            mostrar_token(".", "PUNTO/ACCESO A MIEMBRO", numero_linea);
            i++;
            continue;
        }

        /* Identifica el inicio de una directiva del preprocesador. */
        if (linea_texto[i] == '#') {
            mostrar_token("#", "SIMBOLO DE PREPROCESADOR", numero_linea);
            i++;
            continue;
        }

        /* Informa si aparece un caracter fuera de las categorias conocidas. */
        token[0] = linea_texto[i];
        token[1] = '\0';
        mostrar_token(token, "CARACTER NO RECONOCIDO", numero_linea);
        i++;
    }
}

/* Pide el archivo, analiza sus lineas y comprueba errores de lectura. */
int main(void) {
    FILE *archivo;
    char nombre_archivo[MAX_NOMBRE];
    char linea[MAX_LINEA];
    int numero_linea = 0;

    printf("Introduce el nombre del archivo que deseas leer: ");

    if (fgets(nombre_archivo, sizeof(nombre_archivo), stdin) == NULL) {
        fprintf(stderr, "Error al leer el nombre del archivo.\n");
        return 1;
    }

    nombre_archivo[strcspn(nombre_archivo, "\r\n")] = '\0';

    if (nombre_archivo[0] == '\0') {
        fprintf(stderr, "Error: no se introdujo un nombre de archivo.\n");
        return 1;
    }

    archivo = fopen(nombre_archivo, "r");

    if (archivo == NULL) {
        fprintf(stderr, "Error al abrir el archivo: %s\n", nombre_archivo);
        return 1;
    }

    printf("\nTOKENIZACION DEL ARCHIVO: %s\n", nombre_archivo);
    printf("==============================================\n");

    /* Procesa el archivo linea por linea y conserva el numero de linea. */
    while (fgets(linea, sizeof(linea), archivo) != NULL) {
        numero_linea++;
        analizar_linea(linea, numero_linea);
    }

    if (ferror(archivo)) {
        fprintf(stderr, "Error durante la lectura del archivo.\n");
        fclose(archivo);
        return 1;
    }

    if (en_comentario_bloque)
        printf("Aviso: comentario de bloque sin cerrar al final del archivo.\n");

    fclose(archivo);

    printf("==============================================\n");
    printf("Tokenizacion finalizada.\n");

    return 0;
}

