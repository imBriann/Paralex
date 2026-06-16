import os
import PyPDF2
import docx
import shutil

def extract_text(file_path, output_dir):
    """
    Extrae texto de PDF o DOCX y lo guarda como un archivo .txt en output_dir.
    Retorna la ruta del archivo .txt generado.
    """
    os.makedirs(output_dir, exist_ok=True)
    base_name = os.path.basename(file_path)
    name_without_ext, ext = os.path.splitext(base_name)
    ext = ext.lower()
    
    txt_path = os.path.join(output_dir, f"{name_without_ext}.txt")
    
    try:
        if ext == '.pdf':
            with open(file_path, 'rb') as f:
                reader = PyPDF2.PdfReader(f)
                text = ""
                for page in reader.pages:
                    text += page.extract_text() + "\n"
            with open(txt_path, 'w', encoding='utf-8') as f:
                f.write(text)
                
        elif ext == '.docx':
            doc = docx.Document(file_path)
            text = "\n".join([para.text for para in doc.paragraphs])
            with open(txt_path, 'w', encoding='utf-8') as f:
                f.write(text)
                
        elif ext == '.txt':
            # Simplemente copiar
            shutil.copy(file_path, txt_path)
            
        else:
            raise ValueError(f"Formato no soportado: {ext}")
            
        return txt_path
    except Exception as e:
        print(f"Error extrayendo texto de {file_path}: {e}")
        return None

def update_manifest(txt_paths, manifest_path):
    """
    Reconstruye el archivo manifest.txt usado por el motor MPI C++.
    """
    os.makedirs(os.path.dirname(manifest_path), exist_ok=True)
    with open(manifest_path, 'w', encoding='utf-8') as f:
        f.write("# id\tname\tcorpus_path\tformat\tsize_bytes\n")
        for i, path in enumerate(txt_paths, 1):
            name = os.path.basename(path).replace('.txt', '')
            # Usar rutas absolutas o relativas al data_dir
            f.write(f"{i}\t{name}\t{path}\ttxt\t0\n")
