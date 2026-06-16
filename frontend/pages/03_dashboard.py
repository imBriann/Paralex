import streamlit as st
import os
import sys
import pandas as pd
import plotly.express as px
from wordcloud import WordCloud
import matplotlib.pyplot as plt

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), "..")))
from utils.mpi_bridge import MPIBridge

st.set_page_config(page_title="Dashboard | Paralex", page_icon="📊", layout="wide")

css_path = os.path.join(os.path.dirname(__file__), "..", "utils", "style.css")
if os.path.exists(css_path):
    with open(css_path) as f:
        st.markdown(f"<style>{f.read()}</style>", unsafe_allow_html=True)

st.title("📊 Dashboard Ejecutivo")

bridge = MPIBridge()
df_docs = bridge.get_documents_df()
df_history = bridge.get_execution_history()

if df_docs.empty:
    st.info("Sube documentos para visualizar métricas.")
    st.stop()

# --- KPI CARDS ---
st.markdown("### Métricas Generales")
col1, col2, col3, col4 = st.columns(4)
col1.metric("Total Documentos", len(df_docs))
col2.metric("Total Palabras", f"{df_docs['word_count'].sum():,}")
col3.metric("Términos Únicos", f"{df_docs['unique_terms'].sum():,}")

# If we have execution history, show the latest time
if not df_history.empty:
    latest = df_history.iloc[0]
    col4.metric("Último Tiempo de Procesamiento", f"{latest['time_ms']:.2f} ms")
else:
    col4.metric("Último Tiempo de Procesamiento", "N/A")

st.markdown("---")

# --- NUBES DE PALABRAS ---
st.markdown("### ☁️ Word Cloud Avanzada")

st.info("Haz clic en **Ejecutar Análisis TF-IDF** para obtener las palabras más relevantes del corpus y generar las nubes de palabras interactivas.")
col_btn, col_proc = st.columns([1, 4])
num_processes = col_proc.number_input("Nodos MPI para TF-IDF", min_value=1, max_value=16, value=4, key="tfidf_nodes")

if col_btn.button("Ejecutar Análisis TF-IDF", use_container_width=True):
    bridge.num_processes = num_processes
    with st.spinner(f"Calculando pesos TF-IDF en paralelo con {num_processes} procesos..."):
        res = bridge.tfidf()
        if "error" in res:
            st.error(res["error"])
        else:
            keywords = res.get("keywords", {})
            
            # Generar Word Cloud Global consolidando los top words
            global_freq = {}
            for doc_id, kws in keywords.items():
                for kw in kws:
                    word = kw["word"]
                    score = kw["score"]
                    global_freq[word] = global_freq.get(word, 0) + score
            
            if global_freq:
                st.markdown("#### Nube de Palabras Global (Basado en TF-IDF)")
                wc = WordCloud(width=800, height=400, background_color="white", colormap="viridis").generate_from_frequencies(global_freq)
                fig, ax = plt.subplots(figsize=(10, 5))
                ax.imshow(wc, interpolation='bilinear')
                ax.axis("off")
                st.pyplot(fig)
            
            st.markdown("#### Nubes por Documento")
            doc_tabs = st.tabs([kw[0].get("doc_name", f"Doc {d_id}") for d_id, kw in keywords.items() if kw])
            
            idx = 0
            for doc_id, kws in keywords.items():
                if not kws: continue
                with doc_tabs[idx]:
                    doc_freq = {kw["word"]: kw["score"] for kw in kws}
                    
                    col_chart, col_wc = st.columns(2)
                    with col_chart:
                        df = pd.DataFrame(kws)
                        fig_bar = px.bar(df, x='word', y='score', title="Top TF-IDF", color='score', color_continuous_scale='Blues')
                        st.plotly_chart(fig_bar, use_container_width=True, key=f"bar_{doc_id}")
                    
                    with col_wc:
                        wc_doc = WordCloud(width=400, height=300, background_color="white", colormap="plasma").generate_from_frequencies(doc_freq)
                        fig2, ax2 = plt.subplots()
                        ax2.imshow(wc_doc, interpolation='bilinear')
                        ax2.axis("off")
                        st.pyplot(fig2)
                idx += 1

# --- TENDENCIAS ---
if not df_history.empty:
    st.markdown("---")
    st.markdown("### 📈 Tendencias Históricas")
    fig_hist = px.line(df_history, x='date', y='time_ms', title="Evolución del Tiempo de Procesamiento MapReduce (ms)", markers=True)
    st.plotly_chart(fig_hist, use_container_width=True)
