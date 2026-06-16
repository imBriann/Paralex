import os
import sys
import json
from fastapi import FastAPI, HTTPException, UploadFile, File, BackgroundTasks
from fastapi.middleware.cors import CORSMiddleware
from fastapi.staticfiles import StaticFiles
from fastapi.responses import FileResponse
from pydantic import BaseModel
from typing import List, Optional

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), "..")))
from frontend.utils.mpi_bridge import MPIBridge
from frontend.utils.text_extractor import extract_text, update_manifest

app = FastAPI(title="Paralex API")

app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)

frontend_path = os.path.join(os.path.dirname(__file__), "..", "frontend_web")
os.makedirs(frontend_path, exist_ok=True)
app.mount("/static", StaticFiles(directory=frontend_path), name="static")

@app.get("/")
def serve_index():
    return FileResponse(os.path.join(frontend_path, "index.html"))


bridge = MPIBridge(num_processes=4)

class QueryModel(BaseModel):
    query: str
    nodes: Optional[int] = 4

class ConfigModel(BaseModel):
    nodes: int

@app.get("/api/documents")
def get_documents():
    df = bridge.get_documents_df()
    return df.to_dict(orient="records")

@app.get("/api/documents/{doc_id}")
def get_document_content(doc_id: int):
    content = bridge.get_document_content(doc_id)
    if not content:
        raise HTTPException(status_code=404, detail="Document not found")
    return {"content": content}

@app.get("/api/history")
def get_history():
    df = bridge.get_execution_history()
    return df.to_dict(orient="records")

@app.post("/api/search")
def search(params: QueryModel):
    bridge.num_processes = params.nodes
    res = bridge.search(params.query)
    if "error" in res:
        raise HTTPException(status_code=500, detail=res["error"])
    
    correction = bridge.suggest_correction(params.query)
    if correction and correction.lower() != params.query.lower():
        res["correction"] = correction
    
    return res

@app.post("/api/tfidf")
def tfidf(params: ConfigModel):
    bridge.num_processes = params.nodes
    res = bridge.tfidf()
    if "error" in res:
        raise HTTPException(status_code=500, detail=res["error"])
    return res

@app.post("/api/similarity")
def similarity(params: ConfigModel):
    bridge.num_processes = params.nodes
    res = bridge.similarity()
    if "error" in res:
        raise HTTPException(status_code=500, detail=res["error"])
    return res

@app.post("/api/benchmark")
def benchmark(params: ConfigModel):
    bridge.num_processes = params.nodes
    res = bridge.benchmark()
    if "error" in res:
        raise HTTPException(status_code=500, detail=res["error"])
    return res

@app.post("/api/upload")
async def upload_files(files: List[UploadFile] = File(...)):
    raw_dir = os.path.join(bridge.data_dir, "raw_uploads")
    txt_dir = os.path.join(bridge.data_dir, "sample_docs")
    os.makedirs(raw_dir, exist_ok=True)
    os.makedirs(txt_dir, exist_ok=True)
    
    txt_paths = []
    for file in files:
        file_path = os.path.join(raw_dir, file.filename)
        with open(file_path, "wb") as f:
            f.write(await file.read())
        txt_path = extract_text(file_path, txt_dir)
        if txt_path:
            txt_paths.append(txt_path)
            
    manifest_path = os.path.join(bridge.data_dir, "manifest.txt")
    update_manifest(txt_paths, manifest_path)
    
    res = bridge.process()
    if "error" in res:
        raise HTTPException(status_code=500, detail=res["error"])
    return {"message": "Files processed successfully", "result": res}

@app.post("/api/demo")
def generate_demo_data():
    from frontend.pages.07_modo_demo import generar_corpus_sintetico
    
    txt_dir = os.path.join(bridge.data_dir, "sample_docs")
    os.makedirs(txt_dir, exist_ok=True)
    
    generar_corpus_sintetico(txt_dir, 15)
    
    txt_paths = [os.path.join(txt_dir, f) for f in os.listdir(txt_dir) if f.endswith(".txt")]
    manifest_path = os.path.join(bridge.data_dir, "manifest.txt")
    update_manifest(txt_paths, manifest_path)
    
    res = bridge.process()
    if "error" in res:
        raise HTTPException(status_code=500, detail=res["error"])
    return {"message": "Demo data generated and processed"}
