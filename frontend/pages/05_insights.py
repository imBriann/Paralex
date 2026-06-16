import streamlit as st
import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), "..")))
from utils.mpi_bridge import MPIBridge

st.set_page_config(page_title="Insights | Paralex", page_icon="💡", layout="wide")

css_path = os.path.join(os.path.dirname(__file__), "..", "utils", "style.css")
if os.path.exists(css_path):
    with open(css_path) as f:
        st.markdown(f"<style>{f.read()}</style>", unsafe_allow_html=True)

st.title("💡 Centro de Insights")

bridge = MPIBridge(num_processes=4)
df_docs = bridge.get_documents_df()

if df_docs.empty:
    st.info("Sube documentos en el Explorador para generar insights.")
    st.stop()

st.markdown("Generación de *Insights* automáticos extraídos de los documentos más relevantes de tu corpus.")

if st.button("Generar Insights", use_container_width=True):
    with st.spinner("Analizando corpus con MapReduce para extracción de Insights..."):
        res = bridge.tfidf()
        if "error" in res:
            st.error(res["error"])
        else:
            keywords = res.get("keywords", {})
            st.success("Insights generados exitosamente.")
            
            for doc_id, kws in keywords.items():
                if not kws: continue
                doc_name = kws[0].get("doc_name", f"Doc {doc_id}")
                
                # Insight generativo basado en top words
                top_words = [kw['word'] for kw in kws[:5]]
                
                # Fetch first few lines of document for context
                content = bridge.get_document_content(int(doc_id))
                preview = content[:200].replace('\n', ' ') + "..." if content else "Sin contenido."
                
                st.markdown(f"""
                <div class="css-12oz5g7">
                    <h4 style="color: #0071e3; margin-bottom: 0.5rem;">📄 {doc_name}</h4>
                    <p style="font-size: 0.95rem;"><b>Temas clave detectados:</b> {', '.join(top_words)}</p>
                    <div style="background-color: #f5f5f7; padding: 1rem; border-left: 4px solid #0071e3; border-radius: 4px;">
                        <p style="margin: 0; font-style: italic; color: #555;">"{preview}"</p>
                    </div>
                </div>
                """, unsafe_allow_html=True)
