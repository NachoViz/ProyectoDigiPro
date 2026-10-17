import customtkinter as ctk
import serial
import serial.tools.list_ports
import threading
import re

# --- CONFIGURACIÓN ---
BAUD_RATE = 9600
PUERTO_COM = "COM5"  # <--- ¡CAMBIA ESTO POR TU PUERTO! (Ej: COM3, /dev/ttyUSB0)

class ServoCard(ctk.CTkFrame):
    """Widget personalizado para mostrar un solo servo"""
    def __init__(self, master, title="Servo"):
        super().__init__(master)
        
        # Título del Servo (ej. "Servo 1")
        self.lbl_title = ctk.CTkLabel(self, text=title, font=("Arial", 14, "bold"))
        self.lbl_title.pack(pady=(10, 5))
        
        # Valor del Pulso (ROJO y GRANDE)
        self.lbl_value = ctk.CTkLabel(self, text="0", font=("Impact", 40), text_color="#ff4444")
        self.lbl_value.pack(pady=0)
        
        # Unidad
        self.lbl_unit = ctk.CTkLabel(self, text="µs", font=("Arial", 12))
        self.lbl_unit.pack(pady=(0, 15))

    def update_value(self, value):
        self.lbl_value.configure(text=str(value))

class App(ctk.CTk):
    def __init__(self):
        super().__init__()

        # Configuración de la ventana
        self.title("Monitor de Servos")
        self.geometry("700x400")
        ctk.set_appearance_mode("Dark")
        
        # Variables de datos
        self.ser = None
        self.is_running = False
        self.data_servos = [1500, 1500, 1500] # Valores iniciales

        # --- INTERFAZ GRÁFICA ---
        
        # 1. Título Principal "UART"
        self.lbl_main_title = ctk.CTkLabel(self, text="UART", font=("Arial Black", 30))
        self.lbl_main_title.pack(pady=20)

        # 2. Contenedor para las 3 tarjetas
        self.frame_cards = ctk.CTkFrame(self, fg_color="transparent")
        self.frame_cards.pack(pady=10, padx=20, fill="x")

        # Crear las 3 tarjetas (Servo 1, 2 y 3)
        self.card1 = ServoCard(self.frame_cards, title="Pulso 1")
        self.card1.pack(side="left", expand=True, padx=10, fill="both")

        self.card2 = ServoCard(self.frame_cards, title="Pulso 2")
        self.card2.pack(side="left", expand=True, padx=10, fill="both")

        self.card3 = ServoCard(self.frame_cards, title="Pulso 3")
        self.card3.pack(side="left", expand=True, padx=10, fill="both")

        # 3. Botón Conectar
        self.btn_connect = ctk.CTkButton(self, text="Conectar", command=self.toggle_connection, height=40)
        self.btn_connect.pack(pady=30)

        # 4. Etiqueta de estado (para ver el texto crudo si falla el parseo)
        self.lbl_status = ctk.CTkLabel(self, text="Desconectado", text_color="gray")
        self.lbl_status.pack(side="bottom", pady=10)

        # Iniciar loop de actualización de UI
        self.after(100, self.update_ui)

    def toggle_connection(self):
        if not self.is_running:
            try:
                self.ser = serial.Serial(PUERTO_COM, BAUD_RATE, timeout=1)
                self.is_running = True
                self.btn_connect.configure(text="Desconectar", fg_color="#c0392b") # Rojo oscuro
                self.lbl_status.configure(text="Conectado esperando datos...")
                
                # Hilo separado para leer el puerto serie
                threading.Thread(target=self.read_serial, daemon=True).start()
            except Exception as e:
                self.lbl_status.configure(text=f"Error: {e}")
        else:
            self.is_running = False
            if self.ser:
                self.ser.close()
            self.btn_connect.configure(text="Conectar", fg_color="#1f6aa5") # Azul original
            self.lbl_status.configure(text="Desconectado")

    def read_serial(self):
        # Expresión regular para buscar los números en tu mensaje:
        # "Posicion de los servos : pulso=1500  pulso2=1600  pulso3=1700"
        pattern = re.compile(r"pulso=(\d+).*pulso2=(\d+).*pulso3=(\d+)")

        while self.is_running:
            if self.ser and self.ser.in_waiting > 0:
                try:
                    line = self.ser.readline().decode('utf-8', errors='ignore').strip()
                    
                    # Intentar extraer los números
                    match = pattern.search(line)
                    if match:
                        # Guardar los valores en la lista
                        self.data_servos = [match.group(1), match.group(2), match.group(3)]
                        self.raw_msg = "Datos recibidos OK"
                    else:
                        self.raw_msg = f"Formato desconocido: {line[:30]}..."
                except:
                    pass

    def update_ui(self):
        # Actualizar las tarjetas con los últimos valores leídos
        self.card1.update_value(self.data_servos[0])
        self.card2.update_value(self.data_servos[1])
        self.card3.update_value(self.data_servos[2])
        
        if self.is_running:
             # Mostrar estado si está conectado
             if hasattr(self, 'raw_msg'):
                 self.lbl_status.configure(text=self.raw_msg)

        self.after(50, self.update_ui) # Repetir cada 50ms

if __name__ == "__main__":
    app = App()
    app.mainloop()