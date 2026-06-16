import streamlit as st
import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), "..")))
from utils.mpi_bridge import MPIBridge
from utils.text_extractor import update_manifest

st.set_page_config(page_title="Modo Demo | Paralex", page_icon="🎬", layout="wide")

css_path = os.path.join(os.path.dirname(__file__), "..", "utils", "style.css")
if os.path.exists(css_path):
    with open(css_path) as f:
        st.markdown(f"<style>{f.read()}</style>", unsafe_allow_html=True)

st.title("🎬 Modo Demostración")

st.markdown("""
En esta sección puedes auto-generar un corpus de documentos de prueba y procesarlos automáticamente para explorar la plataforma sin tener que subir archivos manualmente.
""")

num_docs = st.slider("Cantidad de documentos sintéticos a generar", 10, 100, 20)

bridge = MPIBridge(num_processes=4)

if st.button("🚀 Iniciar Modo Demostración", use_container_width=True):
    with st.spinner(f"Generando {num_docs} documentos sintéticos..."):
        txt_dir = os.path.join(bridge.data_dir, "sample_docs")
        os.makedirs(txt_dir, exist_ok=True)
        
        txt_paths = []
        for i in range(1, num_docs + 1):
            path = os.path.join(txt_dir, f"demo_doc_{i}.txt")
            with open(path, "w", encoding="utf-8") as f:
                if i % 3 == 0:
                    f.write("MapReduce es un modelo de programación para procesar grandes conjuntos de datos de forma distribuida en clústeres. ")
                    f.write("MPI se utiliza para pasar mensajes entre nodos. ")
                elif i % 3 == 1:
                    f.write("La computación paralela acelera el tiempo de procesamiento dividiendo tareas. ")
                    f.write("Speedup y eficiencia son métricas clave en el rendimiento paralelo. ")
                else:
                    f.write("La inteligencia artificial y el aprendizaje automático dependen de grandes cantidades de datos. ")
                    f.write("Los motores de búsqueda usan TF-IDF para medir la relevancia de las palabras clave. ")
            txt_paths.append(path)
            
        manifest_path = os.path.join(bridge.data_dir, "manifest.txt")
        update_manifest(txt_paths, manifest_path)
    
    with st.spinner("Procesando documentos con MapReduce..."):
        res = bridge.process()
        if "error" in res:
            st.error(res["error"])
        else:
            st.success("¡Demostración lista! Explora el resto de las pestañas en el panel lateral.")
            st.rerun()
