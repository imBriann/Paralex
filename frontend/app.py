import streamlit as st
import os

st.set_page_config(
    page_title="Paralex MapReduce",
    page_icon="⚡",
    layout="wide",
    initial_sidebar_state="expanded"
)

# Load custom CSS
css_path = os.path.join(os.path.dirname(__file__), "utils", "style.css")
if os.path.exists(css_path):
    with open(css_path) as f:
        st.markdown(f"<style>{f.read()}</style>", unsafe_allow_html=True)

st.markdown("<h1 style='text-align: center; font-weight: 700; font-size: 3rem; margin-bottom: 0;'>Paralex</h1>", unsafe_allow_html=True)
st.markdown("<p style='text-align: center; color: #86868b; font-size: 1.2rem; font-weight: 500; margin-bottom: 2rem;'>Plataforma Profesional de Análisis Documental Distribuido</p>", unsafe_allow_html=True)

col1, col2, col3 = st.columns([1,2,1])
with col2:
    st.markdown("""
    <div style='background-color: #ffffff; border-radius: 12px; padding: 2rem; box-shadow: 0 4px 20px rgba(0,0,0,0.05); text-align: justify;'>
    Bienvenido a <b>Paralex</b>, la plataforma definitiva impulsada por computación paralela (MPI) y arquitectura MapReduce.
    <br><br>
    Desarrollado en <b>C++</b> de alto rendimiento con un ecosistema de visualización moderno. Paralex te permite explorar, indexar y extraer métricas complejas de grandes colecciones de documentos a velocidades sin precedentes.
    </div>
    """, unsafe_allow_html=True)

st.markdown("### 🚀 Características Principales")
cols = st.columns(3)
cols[0].info("**Explorador Documental**\nGestión de archivos tipo Drive con previsualización avanzada.")
cols[1].success("**Dashboard Ejecutivo**\nMétricas en tiempo real, tendencias y Word Clouds interactivas.")
cols[2].warning("**Búsqueda Inteligente**\nMotor de búsqueda con corrección ortográfica y exportación de datos.")

cols2 = st.columns(3)
cols2[0].error("**Análisis de Temas**\nAgrupamiento automático y generación de Insights.")
cols2[1].info("**Rendimiento Paralelo**\nBenchmarks de Speedup y Eficiencia con histórico.")
cols2[2].success("**Similitud Documental**\nMapas de relaciones en grafos interactivos.")

st.markdown("---")
st.markdown("<p style='text-align: center; color: #86868b; font-size: 0.9rem;'>Desarrollado para la materia de Fundamentos de Computación Paralela y Distribuida AR.</p>", unsafe_allow_html=True)

