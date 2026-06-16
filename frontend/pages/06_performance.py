import streamlit as st
import os
import sys
import pandas as pd
import plotly.express as px

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), "..")))
from utils.mpi_bridge import MPIBridge

st.set_page_config(page_title="Rendimiento | Paralex", page_icon="⚡", layout="wide")

css_path = os.path.join(os.path.dirname(__file__), "..", "utils", "style.css")
if os.path.exists(css_path):
    with open(css_path) as f:
        st.markdown(f"<style>{f.read()}</style>", unsafe_allow_html=True)

st.title("⚡ Dashboard de Paralelismo y Rendimiento")

st.markdown("""
Esta herramienta ejecutará el motor MapReduce dos veces:
1. De forma **Secuencial** (1 proceso simulado sin sobrecarga de red).
2. De forma **Paralela** con el número de procesos indicado usando `mpiexec`.
""")

num_processes = st.number_input("Número de procesos MPI", min_value=2, max_value=16, value=4)
bridge = MPIBridge(num_processes=num_processes)

if st.button("Iniciar Benchmark", use_container_width=True):
    with st.spinner("Ejecutando pruebas de rendimiento (puede tardar un momento)..."):
        res = bridge.benchmark()
        
        if "error" in res:
            st.error(res["error"])
        else:
            st.success("Benchmark Completado con Éxito")
            
            st.markdown("### 📊 Resultados del Benchmark")
            col1, col2, col3 = st.columns(3)
            col1.metric("Tiempo Secuencial", f"{res.get('time_sequential_ms', 0):.2f} ms")
            col2.metric(f"Tiempo Paralelo ({num_processes} procs)", f"{res.get('time_parallel_ms', 0):.2f} ms")
            
            speedup = res.get('speedup', 0)
            eficiencia = res.get('efficiency', 0)
            
            col3.metric("Aceleración (Speedup)", f"{speedup:.2f}x")
            
            st.progress(min(max(eficiencia, 0.0), 1.0))
            st.markdown(f"**Eficiencia del sistema:** `{eficiencia * 100:.1f}%` (Ideal: 100%)")
            
            st.info(f"Sobrecarga de comunicación MPI (Overhead aprox): {res.get('overhead_ms', 0):.2f} ms")

st.divider()

st.markdown("### 📈 Historial de Rendimiento")
df_history = bridge.get_execution_history()

if not df_history.empty:
    fig = px.bar(df_history, x='date', y='time_ms', color='mpi_procs', 
                 title="Tiempos de Procesamiento MapReduce por Fecha y Nodos",
                 labels={'time_ms': 'Tiempo (ms)', 'date': 'Fecha', 'mpi_procs': 'Nodos MPI'})
    st.plotly_chart(fig, use_container_width=True)
else:
    st.info("No hay historial de ejecuciones disponible.")
