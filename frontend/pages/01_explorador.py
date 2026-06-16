import streamlit as st
import os
import sys
import pandas as pd

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), "..")))
from utils.text_extractor import extract_text, update_manifest
from utils.mpi_bridge import MPIBridge

st.set_page_config(page_title="Explorador | Paralex", page_icon="📂", layout="wide")

# Load CSS
css_path = os.path.join(os.path.dirname(__file__), "..", "utils", "style.css")
if os.path.exists(css_path):
    with open(css_path) as f:
        st.markdown(f"<style>{f.read()}</style>", unsafe_allow_html=True)

st.title("📂 Explorador Documental")

bridge = MPIBridge(num_processes=4)
df_docs = bridge.get_documents_df()

# --- UPLOAD SECTION ---
with st.expander("➕ Cargar Nuevos Documentos y Ejecutar MapReduce"):
    uploaded_files = st.file_uploader("Arrastra tus archivos aquí (TXT, PDF, DOCX)", accept_multiple_files=True)
    col1, col2 = st.columns([1, 1])
    num_processes = col1.number_input("Nodos MPI", min_value=1, max_value=16, value=4)
    if col2.button("Subir y Procesar", use_container_width=True):
        if uploaded_files:
            with st.spinner("Procesando..."):
                bridge.num_processes = num_processes
                raw_dir = os.path.join(bridge.data_dir, "raw_uploads")
                txt_dir = os.path.join(bridge.data_dir, "sample_docs")
                os.makedirs(raw_dir, exist_ok=True)
                txt_paths = []
                for uploaded_file in uploaded_files:
                    file_path = os.path.join(raw_dir, uploaded_file.name)
                    with open(file_path, "wb") as f:
                        f.write(uploaded_file.getbuffer())
                    txt_path = extract_text(file_path, txt_dir)
                    if txt_path:
                        txt_paths.append(txt_path)
                manifest_path = os.path.join(bridge.data_dir, "manifest.txt")
                update_manifest(txt_paths, manifest_path)
                res = bridge.process()
                if "error" in res:
                    st.error(res["error"])
                else:
                    st.success("Procesamiento completado.")
                    st.rerun()

st.divider()

col_title, col_clear = st.columns([3, 1])
with col_clear:
    if st.button("🗑️ Limpiar Corpus", help="Elimina todos los documentos y la base de datos", use_container_width=True):
        bridge.clear_corpus()
        st.success("Corpus limpiado exitosamente.")
        st.rerun()

if df_docs.empty:
    st.info("No hay documentos indexados. Sube algunos archivos para comenzar.")
else:
    # --- EXPLORER SECTION ---
    col_search, col_view = st.columns([3, 1])
    search_term = col_search.text_input("🔍 Buscar documento por nombre...", "")
    view_type = col_view.radio("Vista", ["Tabla", "Tarjetas"], horizontal=True)

    if search_term:
        df_docs = df_docs[df_docs['name'].str.contains(search_term, case=False)]

    if view_type == "Tabla":
        st.dataframe(df_docs, use_container_width=True, hide_index=True)
    else:
        # Cards View
        cols = st.columns(4)
        for i, row in df_docs.iterrows():
            with cols[i % 4]:
                st.markdown(f"""
                <div class="css-12oz5g7">
                    <h4 style="margin-bottom:0.5rem; white-space: nowrap; overflow: hidden; text-overflow: ellipsis;" title="{row['name']}">📄 {row['name']}</h4>
                    <p style="font-size:0.8rem; color:#86868b; margin-bottom:0.2rem;">{row.get('upload_date', 'N/A')}</p>
                    <p style="font-size:0.8rem; margin-bottom:0.2rem;"><b>Tamaño:</b> {row.get('size', 0)/1024:.1f} KB</p>
                    <p style="font-size:0.8rem; margin-bottom:0.2rem;"><b>Palabras:</b> {row.get('word_count', 0)}</p>
                    <p style="font-size:0.8rem; margin-bottom:0;"><b>Términos:</b> {row.get('unique_terms', 0)}</p>
                </div>
                """, unsafe_allow_html=True)

    st.markdown("### 👁️ Visor Documental")
    doc_to_view = st.selectbox("Selecciona un documento para ver su contenido:", [""] + df_docs['name'].tolist())
    
    if doc_to_view:
        doc_row = df_docs[df_docs['name'] == doc_to_view].iloc[0]
        content = bridge.get_document_content(int(doc_row['doc_id']))
        
        st.markdown(f"**Metadatos:** {doc_row.get('word_count')} palabras | {doc_row.get('unique_terms')} términos únicos | Subido el {doc_row.get('upload_date')}")
        st.markdown("<div style='background: #fff; padding: 2rem; border-radius: 8px; border: 1px solid #eee; height: 400px; overflow-y: auto;'>", unsafe_allow_html=True)
        st.text(content)
        st.markdown("</div>", unsafe_allow_html=True)

