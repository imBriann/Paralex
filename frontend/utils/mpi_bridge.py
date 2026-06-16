import os
import subprocess
import json
import sqlite3
import pandas as pd
import difflib

class MPIBridge:
    def __init__(self, num_processes=4):
        # Asume que el backend compilado está en bin/
        self.base_dir = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
        self.exe_path = os.path.join(self.base_dir, "bin", "paralex_engine.exe")
        self.data_dir = os.path.join(self.base_dir, "data")
        self.results_dir = os.path.join(self.data_dir, "results")
        self.db_path = os.path.join(self.base_dir, "paralex.db")
        self.num_processes = num_processes
        os.makedirs(self.results_dir, exist_ok=True)

    def _run_mpi(self, command, *args):
        output_file = os.path.join(self.results_dir, f"{command}_output.json")
        cmd = [
            "mpiexec", "-n", str(self.num_processes),
            self.exe_path, command, self.data_dir, output_file
        ] + list(args)
        
        try:
            print(f"Running MPI: {' '.join(cmd)}")
            result = subprocess.run(cmd, capture_output=True, text=True, check=True)
            print("MPI Output:", result.stdout)
            
            if os.path.exists(output_file):
                with open(output_file, 'r', encoding='utf-8') as f:
                    return json.load(f)
            else:
                return {"error": "Archivo JSON no generado"}
        except subprocess.CalledProcessError as e:
            print(f"Error MPI: {e.stderr}")
            return {"error": f"Error ejecutando MPI: {e.stderr}"}

    def process(self):
        return self._run_mpi("process")

    def tfidf(self):
        return self._run_mpi("tfidf")

    def search(self, query):
        return self._run_mpi("search", query)

    def similarity(self):
        return self._run_mpi("similarity")

    def benchmark(self):
        return self._run_mpi("benchmark")

    def get_documents_df(self):
        if not os.path.exists(self.db_path):
            return pd.DataFrame()
        conn = sqlite3.connect(self.db_path)
        df = pd.read_sql_query("SELECT doc_id, name, size, upload_date, word_count, unique_terms, tags FROM documents", conn)
        conn.close()
        return df

    def get_document_content(self, doc_id):
        if not os.path.exists(self.db_path):
            return ""
        conn = sqlite3.connect(self.db_path)
        cursor = conn.cursor()
        cursor.execute("SELECT content FROM documents WHERE doc_id = ?", (doc_id,))
        row = cursor.fetchone()
        conn.close()
        return row[0] if row else ""

    def get_execution_history(self):
        if not os.path.exists(self.db_path):
            return pd.DataFrame()
        conn = sqlite3.connect(self.db_path)
        try:
            df = pd.read_sql_query("SELECT * FROM execution_history ORDER BY exec_id DESC", conn)
        except:
            df = pd.DataFrame()
        conn.close()
        return df

    def suggest_correction(self, query):
        if not os.path.exists(self.db_path):
            return None
        conn = sqlite3.connect(self.db_path)
        cursor = conn.cursor()
        try:
            cursor.execute("SELECT DISTINCT term FROM inverted_index")
            terms = [row[0] for row in cursor.fetchall()]
        except:
            terms = []
        conn.close()
        
        words = query.split()
        corrected_words = []
        changed = False
        for word in words:
            if word not in terms and len(terms) > 0:
                matches = difflib.get_close_matches(word, terms, n=1, cutoff=0.7)
                if matches:
                    corrected_words.append(matches[0])
                    changed = True
                else:
                    corrected_words.append(word)
            else:
                corrected_words.append(word)
        
        if changed:
            return " ".join(corrected_words)
        return None

    def clear_corpus(self):
        import shutil
        raw_dir = os.path.join(self.data_dir, "raw_uploads")
        txt_dir = os.path.join(self.data_dir, "sample_docs")
        manifest_path = os.path.join(self.data_dir, "manifest.txt")
        
        # Delete directories
        if os.path.exists(raw_dir):
            shutil.rmtree(raw_dir)
        if os.path.exists(txt_dir):
            shutil.rmtree(txt_dir)
            
        # Recreate empty directories
        os.makedirs(raw_dir, exist_ok=True)
        os.makedirs(txt_dir, exist_ok=True)
        
        # Delete manifest
        if os.path.exists(manifest_path):
            os.remove(manifest_path)
            
        # Create empty manifest
        with open(manifest_path, 'w', encoding='utf-8') as f:
            f.write("")
            
        # Delete database
        if os.path.exists(self.db_path):
            os.remove(self.db_path)
            
        return {"status": "success", "message": "Corpus cleared successfully"}
