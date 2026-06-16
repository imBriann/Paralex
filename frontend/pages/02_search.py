import streamlit as st
import os
import sys
import pandas as pd

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), "..")))
from utils.mpi_bridge import MPIBridge

st.set_page_config(page_title="Búsqueda | Paralex", page_icon="🔍", layout="wide")

css_path = os.path.join(os.path.dirname(__file__), "..", "utils", "style.css")
if os.path.exists(css_path):
    with open(css_path) as f:
        st.markdown(f"<style>{f.read()}</style>", unsafe_allow_html=True)

st.title("🔍 Búsqueda Inteligente")

bridge = MPIBridge(num_processes=4)

query = st.text_input("Ingresa tu consulta para buscar en todo el corpus documental:", placeholder="Ej. paralelización MPI")

if query:
    correction = bridge.suggest_correction(query)
    if correction and correction.lower() != query.lower():
        st.info(f"💡 ¿Quisiste decir: **{correction}**?")
        if st.button(f"Buscar '{correction}' en su lugar"):
            query = correction

col1, col2 = st.columns([1, 5])
num_processes = col1.number_input("Procesos MPI", min_value=1, max_value=16, value=4)

if col1.button("Buscar", use_container_width=True):
    if query:
        bridge.num_processes = num_processes
        with st.spinner(f"Buscando en el clúster con {num_processes} procesos..."):
            res = bridge.search(query)
            
            if "error" in res:
                st.error(res["error"])
            else:
                st.success(f"Resultados obtenidos en {res.get('processing_time_ms', 0):.2f} ms")
                results = res.get("results", [])
                st.markdown(f"**Se encontraron {len(results)} documentos relevantes.**")
                
                if results:
                    df_results = pd.DataFrame(results)
                    # For Export
                    csv = df_results.to_csv(index=False).encode('utf-8')
                    st.download_button(
                        label="📥 Exportar Resultados a CSV",
                        data=csv,
                        file_name='resultados_busqueda.csv',
                        mime='text/csv',
                    )
                
                st.markdown("---")
                for r in results:
                    st.markdown(f"""
                    <div class="css-1r6slb0">
                        <h3 style="margin-bottom: 0.5rem; color: #0071e3;">📄 {r['doc_name']}</h3>
                        <p style="font-size: 0.9rem; color: #86868b; margin-bottom: 0.5rem;">Document ID: {r['doc_id']}</p>
                        <p style="margin-bottom: 0;"><b>Relevancia TF-IDF:</b> {r['score']:.4f} &nbsp;&nbsp;|&nbsp;&nbsp; <b>Apariciones:</b> {r['match_count']}</p>
                    </div>
                    """, unsafe_allow_html=True)
    else:
        st.warning("Ingresa una consulta para buscar.")

