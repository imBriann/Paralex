const API_URL = "http://localhost:8000/api";

function navigate(viewId) {
    // Hide all views
    document.querySelectorAll('.view').forEach(view => {
        view.classList.remove('active');
    });
    
    // Deactivate nav links
    document.querySelectorAll('.nav-links li').forEach(li => {
        li.classList.remove('active');
    });
    
    // Show selected view
    document.getElementById(`view-${viewId}`).classList.add('active');
    
    // Update nav link and title
    const clickedLink = Array.from(document.querySelectorAll('.nav-links li')).find(li => li.getAttribute('onclick').includes(viewId));
    if (clickedLink) {
        clickedLink.classList.add('active');
        document.getElementById('page-title').innerText = clickedLink.innerText.trim();
    }
    
    // Load data based on view
    if (viewId === 'explorer') loadDocuments();
    if (viewId === 'dashboard') loadKPIs();
}

// --- EXPLORER ---
async function loadDocuments() {
    try {
        const res = await fetch(`${API_URL}/documents`);
        const docs = await res.json();
        
        const tbody = document.querySelector('#docs-table tbody');
        tbody.innerHTML = '';
        
        docs.forEach(doc => {
            const sizeKB = (doc.size / 1024).toFixed(1);
            tbody.innerHTML += `
                <tr>
                    <td><strong>📄 ${doc.name}</strong></td>
                    <td>${sizeKB} KB</td>
                    <td>${doc.upload_date}</td>
                    <td>${doc.word_count}</td>
                    <td>${doc.unique_terms}</td>
                </tr>
            `;
        });
    } catch (e) {
        console.error("Failed to load documents", e);
    }
}

// Drag & Drop Upload
const uploadZone = document.getElementById('upload-zone');
const fileInput = document.getElementById('file-input');

uploadZone.addEventListener('click', () => fileInput.click());

fileInput.addEventListener('change', (e) => handleUpload(e.target.files));

async function handleUpload(files) {
    if (!files.length) return;
    document.getElementById('upload-status').innerHTML = "<p style='color: var(--accent-color)'>Subiendo y procesando...</p>";
    
    const formData = new FormData();
    for (let f of files) formData.append("files", f);
    
    try {
        const res = await fetch(`${API_URL}/upload`, { method: "POST", body: formData });
        const result = await res.json();
        document.getElementById('upload-status').innerHTML = `<p style='color: #34c759'>¡Éxito! ${result.message}</p>`;
        loadDocuments();
    } catch(e) {
        document.getElementById('upload-status').innerHTML = `<p style='color: red'>Error al subir archivos.</p>`;
    }
}

// --- SEARCH ---
async function performSearch() {
    const q = document.getElementById('search-input').value;
    if (!q) return;
    
    try {
        const res = await fetch(`${API_URL}/search`, {
            method: "POST",
            headers: { "Content-Type": "application/json" },
            body: JSON.stringify({ query: q, nodes: 4 })
        });
        const data = await res.json();
        
        const correctionBox = document.getElementById('correction-box');
        if (data.correction) {
            correctionBox.style.display = 'block';
            correctionBox.innerHTML = `💡 ¿Quisiste decir: <strong style="cursor:pointer; color:var(--accent-color)" onclick="applyCorrection('${data.correction}')">${data.correction}</strong>?`;
        } else {
            correctionBox.style.display = 'none';
        }
        
        const grid = document.getElementById('search-results');
        grid.innerHTML = '';
        
        if (data.results && data.results.length > 0) {
            data.results.forEach(r => {
                grid.innerHTML += `
                    <div class="card glass">
                        <h4 style="color: var(--accent-color); margin-bottom:8px">📄 ${r.doc_name}</h4>
                        <p style="font-size: 0.9rem; margin-bottom:4px"><strong>ID:</strong> ${r.doc_id}</p>
                        <p style="font-size: 0.9rem; margin-bottom:4px"><strong>Relevancia:</strong> ${r.score.toFixed(4)}</p>
                        <p style="font-size: 0.9rem;"><strong>Apariciones:</strong> ${r.match_count}</p>
                    </div>
                `;
            });
        } else {
            grid.innerHTML = '<p>No se encontraron resultados.</p>';
        }
        
    } catch(e) {
        console.error(e);
    }
}

function applyCorrection(term) {
    document.getElementById('search-input').value = term;
    performSearch();
}

// --- DASHBOARD ---
async function loadKPIs() {
    try {
        const res = await fetch(`${API_URL}/documents`);
        const docs = await res.json();
        
        document.getElementById('kpi-docs').innerText = docs.length;
        document.getElementById('kpi-words').innerText = docs.reduce((acc, d) => acc + d.word_count, 0);
        document.getElementById('kpi-terms').innerText = docs.reduce((acc, d) => acc + d.unique_terms, 0);
    } catch(e) {}
}

let tfidfChartInstance = null;
async function generateTFIDF() {
    document.getElementById('chart-container').style.display = 'block';
    try {
        const res = await fetch(`${API_URL}/tfidf`, {
            method: "POST",
            headers: { "Content-Type": "application/json" },
            body: JSON.stringify({ nodes: 4 })
        });
        const data = await res.json();
        
        let globalFreq = {};
        for (let doc in data.keywords) {
            data.keywords[doc].forEach(kw => {
                globalFreq[kw.word] = (globalFreq[kw.word] || 0) + kw.score;
            });
        }
        
        // Convert to array and sort
        const sorted = Object.entries(globalFreq).sort((a,b) => b[1] - a[1]).slice(0, 15);
        const labels = sorted.map(i => i[0]);
        const values = sorted.map(i => i[1]);
        
        const ctx = document.getElementById('tfidfChart').getContext('2d');
        if (tfidfChartInstance) tfidfChartInstance.destroy();
        
        tfidfChartInstance = new Chart(ctx, {
            type: 'bar',
            data: {
                labels: labels,
                datasets: [{
                    label: 'Score TF-IDF Global',
                    data: values,
                    backgroundColor: 'rgba(0, 113, 227, 0.7)',
                    borderRadius: 4
                }]
            },
            options: {
                responsive: true,
                maintainAspectRatio: false,
                scales: { y: { beginAtZero: true } }
            }
        });
        
    } catch(e) { console.error(e); }
}

// --- PERFORMANCE ---
async function runBenchmark() {
    const nodes = document.getElementById('mpi-nodes').value;
    document.getElementById('b-seq').innerText = "...";
    document.getElementById('b-par').innerText = "...";
    document.getElementById('benchmark-results').style.display = 'block';
    
    try {
        const res = await fetch(`${API_URL}/benchmark`, {
            method: "POST",
            headers: { "Content-Type": "application/json" },
            body: JSON.stringify({ nodes: parseInt(nodes) })
        });
        const data = await res.json();
        
        document.getElementById('b-seq').innerText = `${data.time_sequential_ms.toFixed(2)} ms`;
        document.getElementById('b-par').innerText = `${data.time_parallel_ms.toFixed(2)} ms`;
        document.getElementById('b-spd').innerText = `${data.speedup.toFixed(2)}x`;
        
    } catch(e) { console.error(e); }
}

// Init
navigate('home');

// --- DEMO MODE ---
async function runDemoMode() {
    document.getElementById('status-indicator').innerText = "Generando...";
    document.getElementById('status-indicator').classList.remove('online');
    
    try {
        const res = await fetch(`${API_URL}/demo`, { method: "POST" });
        await res.json();
        
        document.getElementById('status-indicator').innerText = "Conectado";
        document.getElementById('status-indicator').classList.add('online');
        alert("¡Modo Demo activado! Corpus generado.");
        loadDocuments();
    } catch(e) {
        console.error(e);
        document.getElementById('status-indicator').innerText = "Error";
    }
}

// --- MODAL VIEWER ---
async function openDocumentModal(docId, docName) {
    document.getElementById('modal-title').innerText = docName;
    document.getElementById('modal-body').innerText = "Cargando...";
    document.getElementById('doc-modal').style.display = 'flex';
    
    try {
        const res = await fetch(`${API_URL}/documents/${docId}`);
        const data = await res.json();
        document.getElementById('modal-body').innerText = data.content;
    } catch(e) {
        document.getElementById('modal-body').innerText = "Error al cargar documento.";
    }
}

function closeModal() {
    document.getElementById('doc-modal').style.display = 'none';
}

// Modify loadDocuments to add onClick for the modal
async function loadDocuments() {
    try {
        const res = await fetch(`${API_URL}/documents`);
        const docs = await res.json();
        
        const tbody = document.querySelector('#docs-table tbody');
        tbody.innerHTML = '';
        
        docs.forEach(doc => {
            const sizeKB = (doc.size / 1024).toFixed(1);
            tbody.innerHTML += `
                <tr style="cursor:pointer;" onclick="openDocumentModal(${doc.doc_id}, '${doc.name}')">
                    <td><strong>📄 ${doc.name}</strong></td>
                    <td>${sizeKB} KB</td>
                    <td>${doc.upload_date}</td>
                    <td>${doc.word_count}</td>
                    <td>${doc.unique_terms}</td>
                </tr>
            `;
        });
    } catch (e) { console.error("Failed to load documents", e); }
}

// --- EXPORT CSV ---
let currentSearchResults = [];

async function performSearch() {
    const q = document.getElementById('search-input').value;
    if (!q) return;
    
    try {
        const res = await fetch(`${API_URL}/search`, {
            method: "POST",
            headers: { "Content-Type": "application/json" },
            body: JSON.stringify({ query: q, nodes: 4 })
        });
        const data = await res.json();
        
        const correctionBox = document.getElementById('correction-box');
        if (data.correction) {
            correctionBox.style.display = 'block';
            correctionBox.innerHTML = `💡 ¿Quisiste decir: <strong style="cursor:pointer; color:var(--accent-color)" onclick="applyCorrection('${data.correction}')">${data.correction}</strong>?`;
        } else {
            correctionBox.style.display = 'none';
        }
        
        const grid = document.getElementById('search-results');
        grid.innerHTML = '';
        currentSearchResults = data.results || [];
        
        if (currentSearchResults.length > 0) {
            currentSearchResults.forEach(r => {
                grid.innerHTML += `
                    <div class="card glass" style="cursor:pointer;" onclick="openDocumentModal(${r.doc_id}, '${r.doc_name}')">
                        <h4 style="color: var(--accent-color); margin-bottom:8px">📄 ${r.doc_name}</h4>
                        <p style="font-size: 0.9rem; margin-bottom:4px"><strong>Relevancia:</strong> ${r.score.toFixed(4)}</p>
                        <p style="font-size: 0.9rem;"><strong>Apariciones:</strong> ${r.match_count}</p>
                    </div>
                `;
            });
        } else {
            grid.innerHTML = '<p>No se encontraron resultados.</p>';
        }
        
    } catch(e) { console.error(e); }
}

function exportSearchCSV() {
    if (currentSearchResults.length === 0) {
        alert("No hay resultados para exportar");
        return;
    }
    
    let csv = "ID_Doc,Nombre_Doc,Score_TFIDF,Apariciones\n";
    currentSearchResults.forEach(r => {
        csv += `${r.doc_id},"${r.doc_name}",${r.score},${r.match_count}\n`;
    });
    
    const blob = new Blob([csv], { type: 'text/csv' });
    const url = window.URL.createObjectURL(blob);
    const a = document.createElement('a');
    a.setAttribute('hidden', '');
    a.setAttribute('href', url);
    a.setAttribute('download', 'resultados_busqueda.csv');
    document.body.appendChild(a);
    a.click();
    document.body.removeChild(a);
}

// --- RELATIONS GRAPH ---
async function generateRelations() {
    try {
        const res = await fetch(`${API_URL}/similarity`, {
            method: "POST",
            headers: { "Content-Type": "application/json" },
            body: JSON.stringify({ nodes: 4 })
        });
        const data = await res.json();
        
        const nodes = [];
        const edges = [];
        const nodeIds = new Set();
        
        data.pairs.forEach(pair => {
            if (pair.similarity > 0.1) {
                if (!nodeIds.has(pair.doc1_id)) {
                    nodes.push({ id: pair.doc1_id, label: pair.doc1_name, shape: 'dot', size: 15, color: '#0071e3' });
                    nodeIds.add(pair.doc1_id);
                }
                if (!nodeIds.has(pair.doc2_id)) {
                    nodes.push({ id: pair.doc2_id, label: pair.doc2_name, shape: 'dot', size: 15, color: '#0071e3' });
                    nodeIds.add(pair.doc2_id);
                }
                edges.push({
                    from: pair.doc1_id,
                    to: pair.doc2_id,
                    value: pair.similarity,
                    title: `Similitud: ${(pair.similarity * 100).toFixed(1)}%`
                });
            }
        });
        
        const container = document.getElementById('network-container');
        const netData = { nodes: new vis.DataSet(nodes), edges: new vis.DataSet(edges) };
        const options = {
            physics: {
                barnesHut: { gravitationalConstant: -2000, centralGravity: 0.3, springLength: 95 }
            }
        };
        new vis.Network(container, netData, options);
        
    } catch(e) { console.error(e); }
}

// --- INSIGHTS ---
async function generateInsights() {
    const container = document.getElementById('insights-container');
    container.innerHTML = '<p>Generando insights analíticos...</p>';
    
    try {
        const res = await fetch(`${API_URL}/tfidf`, {
            method: "POST",
            headers: { "Content-Type": "application/json" },
            body: JSON.stringify({ nodes: 4 })
        });
        const data = await res.json();
        
        container.innerHTML = '';
        for (const [docName, words] of Object.entries(data.keywords)) {
            const topWords = words.slice(0, 5).map(w => w.word).join(", ");
            const badgeClass = "background-color: rgba(0, 113, 227, 0.1); color: var(--accent-color); padding: 4px 8px; border-radius: 4px; font-weight: 500; font-size: 0.85rem;";
            
            container.innerHTML += `
                <div style="border-bottom: 1px solid var(--border-color); padding-bottom: 16px;">
                    <h4 style="margin-bottom: 8px;">📘 ${docName}</h4>
                    <p style="font-size: 0.95rem; color: var(--text-secondary);">
                        Este documento se enfoca principalmente en: 
                        <span style="${badgeClass}">${topWords}</span>
                    </p>
                </div>
            `;
        }
        
    } catch(e) { console.error(e); }
}
