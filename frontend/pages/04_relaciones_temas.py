import streamlit as st
import os
import sys
import pandas as pd
import plotly.graph_objects as go
import numpy as np

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), "..")))
from utils.mpi_bridge import MPIBridge

st.set_page_config(page_title="Relaciones | Paralex", page_icon="🧬", layout="wide")

css_path = os.path.join(os.path.dirname(__file__), "..", "utils", "style.css")
if os.path.exists(css_path):
    with open(css_path) as f:
        st.markdown(f"<style>{f.read()}</style>", unsafe_allow_html=True)

st.title("🧬 Mapa de Relaciones y Temas")

bridge = MPIBridge(num_processes=4)

if st.button("Analizar Relaciones y Temas", use_container_width=True):
    with st.spinner("Construyendo grafo de conocimiento..."):
        res = bridge.similarity()
        
        if "error" in res:
            st.error(res["error"])
        else:
            st.success(f"Análisis completado en {res.get('processing_time_ms', 0):.2f} ms")
            
            # --- TABS ---
            tab1, tab2 = st.tabs(["🕸️ Mapa de Relaciones (Grafo)", "📚 Análisis de Temas (Clustering)"])
            
            with tab1:
                st.markdown("### Grafo de Similitud Documental")
                st.info("Visualización estilo red (nodos = documentos, aristas = similitud).")
                
                top_similar = res.get("top_similar", [])
                doc_names = res.get("doc_names", {})
                
                if top_similar:
                    # Crear grafo simple con Plotly
                    node_x = []
                    node_y = []
                    node_text = []
                    
                    # Posiciones aleatorias o circulares
                    doc_ids = res.get("doc_ids", [])
                    angles = np.linspace(0, 2*np.pi, len(doc_ids), endpoint=False)
                    pos = {doc_id: (np.cos(angle), np.sin(angle)) for doc_id, angle in zip(doc_ids, angles)}
                    
                    edge_x = []
                    edge_y = []
                    
                    for sim in top_similar:
                        if sim['similarity'] > 0.1: # Threshold
                            d1 = sim['doc_id_1']
                            d2 = sim['doc_id_2']
                            if d1 in pos and d2 in pos:
                                edge_x.extend([pos[d1][0], pos[d2][0], None])
                                edge_y.extend([pos[d1][1], pos[d2][1], None])
                    
                    for doc_id, name in doc_names.items():
                        if int(doc_id) in pos:
                            node_x.append(pos[int(doc_id)][0])
                            node_y.append(pos[int(doc_id)][1])
                            node_text.append(name)

                    edge_trace = go.Scatter(
                        x=edge_x, y=edge_y,
                        line=dict(width=1, color='#888'),
                        hoverinfo='none',
                        mode='lines')

                    node_trace = go.Scatter(
                        x=node_x, y=node_y,
                        mode='markers+text',
                        text=node_text,
                        textposition="bottom center",
                        hoverinfo='text',
                        marker=dict(
                            showscale=True,
                            colorscale='YlGnBu',
                            size=20,
                            line_width=2))

                    fig = go.Figure(data=[edge_trace, node_trace],
                                 layout=go.Layout(
                                    showlegend=False,
                                    hovermode='closest',
                                    margin=dict(b=20,l=5,r=5,t=40),
                                    xaxis=dict(showgrid=False, zeroline=False, showticklabels=False),
                                    yaxis=dict(showgrid=False, zeroline=False, showticklabels=False))
                                    )
                    st.plotly_chart(fig, use_container_width=True)
                else:
                    st.warning("No hay suficientes relaciones para dibujar el grafo.")
            
            with tab2:
                st.markdown("### Agrupamiento Automático (Temas)")
                # Simulando clustering (En un entorno real usaríamos KMeans de sklearn sobre TF-IDF)
                # Dado que ya tenemos similitud, agruparemos los muy similares
                clusters = []
                visited = set()
                
                for d_id in res.get("doc_ids", []):
                    if d_id in visited: continue
                    cluster = [d_id]
                    visited.add(d_id)
                    for sim in top_similar:
                        if sim['similarity'] > 0.3:
                            if sim['doc_id_1'] == d_id and sim['doc_id_2'] not in visited:
                                cluster.append(sim['doc_id_2'])
                                visited.add(sim['doc_id_2'])
                            elif sim['doc_id_2'] == d_id and sim['doc_id_1'] not in visited:
                                cluster.append(sim['doc_id_1'])
                                visited.add(sim['doc_id_1'])
                    clusters.append(cluster)
                
                for i, cluster in enumerate(clusters):
                    st.markdown(f"""
                    <div class="css-1r6slb0">
                        <h4 style="color: #0071e3;">Tema Principal {i+1}</h4>
                        <p>Documentos asociados:</p>
                        <ul>
                            {''.join(f"<li>{doc_names.get(str(d), str(d))}</li>" for d in cluster)}
                        </ul>
                    </div>
                    """, unsafe_allow_html=True)

