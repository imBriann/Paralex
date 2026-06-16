# ==============================================================================
# Paralex - Setup Script for Windows 11
# Sistema de Procesamiento Paralelo de Documentos con MapReduce/MPI
# ==============================================================================

param(
    [switch]$SkipMPI,
    [switch]$SkipPython,
    [switch]$SkipBuild,
    [switch]$Help
)

$ErrorActionPreference = "Stop"
$ProjectRoot = Split-Path -Parent $MyInvocation.MyCommand.Path

function Write-Header {
    param([string]$Text)
    Write-Host ""
    Write-Host "==============================================================" -ForegroundColor Cyan
    Write-Host "  $Text" -ForegroundColor Cyan
    Write-Host "==============================================================" -ForegroundColor Cyan
    Write-Host ""
}

function Write-Step {
    param([string]$Text)
    Write-Host "[*] $Text" -ForegroundColor Green
}

function Write-Warning {
    param([string]$Text)
    Write-Host "[!] $Text" -ForegroundColor Yellow
}

function Write-Err {
    param([string]$Text)
    Write-Host "[X] $Text" -ForegroundColor Red
}

if ($Help) {
    Write-Host "Paralex Setup Script"
    Write-Host "Usage: .\setup.ps1 [-SkipMPI] [-SkipPython] [-SkipBuild] [-Help]"
    Write-Host ""
    Write-Host "Options:"
    Write-Host "  -SkipMPI     Skip MS-MPI installation check"
    Write-Host "  -SkipPython  Skip Python dependency installation"
    Write-Host "  -SkipBuild   Skip C++ build"
    Write-Host "  -Help        Show this help message"
    exit 0
}

Write-Header "PARALEX - Setup del Entorno"

# ==============================================================================
# 1. Check Python
# ==============================================================================
Write-Header "1. Verificando Python"

$pythonCmd = $null
foreach ($cmd in @("python", "python3", "py")) {
    try {
        $ver = & $cmd --version 2>&1
        if ($ver -match "Python 3") {
            $pythonCmd = $cmd
            Write-Step "Python encontrado: $ver ($cmd)"
            break
        }
    } catch {}
}

if (-not $pythonCmd) {
    Write-Err "Python 3 no encontrado. Instalar desde https://www.python.org/downloads/"
    Write-Host "Asegurate de marcar 'Add Python to PATH' durante la instalacion."
    exit 1
}

# ==============================================================================
# 2. Install Python Dependencies
# ==============================================================================
if (-not $SkipPython) {
    Write-Header "2. Instalando dependencias Python"
    
    $reqFile = Join-Path $ProjectRoot "requirements.txt"
    if (Test-Path $reqFile) {
        Write-Step "Instalando paquetes desde requirements.txt..."
        & $pythonCmd -m pip install --upgrade pip 2>&1 | Out-Null
        & $pythonCmd -m pip install -r $reqFile
        if ($LASTEXITCODE -ne 0) {
            Write-Err "Error instalando dependencias Python"
            exit 1
        }
        Write-Step "Dependencias Python instaladas correctamente"
    } else {
        Write-Warning "requirements.txt no encontrado en $ProjectRoot"
    }
} else {
    Write-Step "Saltando instalacion de dependencias Python (--SkipPython)"
}

# ==============================================================================
# 3. Check MS-MPI
# ==============================================================================
if (-not $SkipMPI) {
    Write-Header "3. Verificando MS-MPI"
    
    $mpiExec = Get-Command mpiexec -ErrorAction SilentlyContinue
    $msmpiInc = $env:MSMPI_INC
    $msmpiLib = $env:MSMPI_LIB64
    
    if ($mpiExec) {
        Write-Step "mpiexec encontrado: $($mpiExec.Source)"
    } else {
        Write-Warning "mpiexec NO encontrado en PATH"
    }
    
    if ($msmpiInc -and (Test-Path $msmpiInc)) {
        Write-Step "MS-MPI SDK encontrado: $msmpiInc"
    } else {
        Write-Warning "MS-MPI SDK NO encontrado (MSMPI_INC no configurado)"
    }
    
    if (-not $mpiExec -or -not $msmpiInc) {
        Write-Host ""
        Write-Host "  MS-MPI no esta completamente instalado." -ForegroundColor Yellow
        Write-Host "  Para instalar MS-MPI en Windows 11:" -ForegroundColor Yellow
        Write-Host ""
        Write-Host "  1. Descargar MS-MPI Runtime y SDK desde:" -ForegroundColor White
        Write-Host "     https://learn.microsoft.com/en-us/message-passing-interface/microsoft-mpi" -ForegroundColor Blue
        Write-Host ""
        Write-Host "  2. Instalar AMBOS:" -ForegroundColor White
        Write-Host "     - msmpisetup.exe  (Runtime - proporciona mpiexec)" -ForegroundColor White
        Write-Host "     - msmpisdk.msi    (SDK - proporciona headers y librerias)" -ForegroundColor White
        Write-Host ""
        Write-Host "  3. Reiniciar la terminal despues de la instalacion" -ForegroundColor White
        Write-Host ""
        Write-Host "  4. Verificar con: mpiexec /?" -ForegroundColor White
        Write-Host ""
        
        $response = Read-Host "Deseas continuar sin MPI? (s/n)"
        if ($response -ne "s" -and $response -ne "S") {
            exit 1
        }
    }
} else {
    Write-Step "Saltando verificacion de MS-MPI (--SkipMPI)"
}

# ==============================================================================
# 4. Check C++ Compiler
# ==============================================================================
Write-Header "4. Verificando compilador C++"

$hasCompiler = $false
$compilerInfo = ""

# Check for cl.exe (MSVC)
$cl = Get-Command cl -ErrorAction SilentlyContinue
if ($cl) {
    $hasCompiler = $true
    $compilerInfo = "MSVC (cl.exe): $($cl.Source)"
}

# Check for g++ (MinGW)
if (-not $hasCompiler) {
    $gpp = Get-Command g++ -ErrorAction SilentlyContinue
    if ($gpp) {
        $hasCompiler = $true
        $compilerInfo = "g++ (MinGW): $($gpp.Source)"
    }
}

# Check for cmake
$cmake = Get-Command cmake -ErrorAction SilentlyContinue

if ($hasCompiler) {
    Write-Step "Compilador encontrado: $compilerInfo"
} else {
    Write-Warning "No se encontro un compilador C++"
    Write-Host ""
    Write-Host "  Opciones para instalar un compilador:" -ForegroundColor Yellow
    Write-Host "  1. Visual Studio Build Tools (recomendado):" -ForegroundColor White
    Write-Host "     https://visualstudio.microsoft.com/visual-cpp-build-tools/" -ForegroundColor Blue
    Write-Host "  2. MinGW-w64 via MSYS2:" -ForegroundColor White
    Write-Host "     https://www.msys2.org/" -ForegroundColor Blue
    Write-Host ""
}

if ($cmake) {
    Write-Step "CMake encontrado: $($cmake.Source)"
} else {
    Write-Warning "CMake no encontrado. Instalar desde https://cmake.org/download/"
}

# ==============================================================================
# 5. Create Directory Structure
# ==============================================================================
Write-Header "5. Creando estructura de directorios"

$dirs = @(
    "data/documents",
    "data/corpus",
    "data/results",
    "data/stopwords",
    "data/sample_docs",
    "build",
    "logs"
)

foreach ($dir in $dirs) {
    $fullPath = Join-Path $ProjectRoot $dir
    if (-not (Test-Path $fullPath)) {
        New-Item -ItemType Directory -Path $fullPath -Force | Out-Null
        Write-Step "Creado: $dir/"
    } else {
        Write-Step "Existe: $dir/"
    }
}

# ==============================================================================
# 6. Initialize SQLite Database
# ==============================================================================
Write-Header "6. Inicializando base de datos SQLite"

$dbPath = Join-Path (Join-Path $ProjectRoot "data") "paralex.db"
$initScript = @"
import sqlite3
import os

db_path = r'$dbPath'
conn = sqlite3.connect(db_path)
c = conn.cursor()

c.executescript('''
    CREATE TABLE IF NOT EXISTS documents (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        name TEXT NOT NULL,
        original_path TEXT,
        format TEXT NOT NULL,
        size_bytes INTEGER,
        word_count INTEGER DEFAULT 0,
        unique_words INTEGER DEFAULT 0,
        lexical_density REAL DEFAULT 0.0,
        processed INTEGER DEFAULT 0,
        created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
    );

    CREATE TABLE IF NOT EXISTS corpus_text (
        doc_id INTEGER PRIMARY KEY,
        content TEXT NOT NULL,
        FOREIGN KEY (doc_id) REFERENCES documents(id)
    );

    CREATE TABLE IF NOT EXISTS word_counts (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        doc_id INTEGER NOT NULL,
        term TEXT NOT NULL,
        count INTEGER NOT NULL,
        tf REAL DEFAULT 0.0,
        FOREIGN KEY (doc_id) REFERENCES documents(id)
    );

    CREATE TABLE IF NOT EXISTS global_counts (
        term TEXT PRIMARY KEY,
        total_count INTEGER NOT NULL,
        doc_frequency INTEGER NOT NULL
    );

    CREATE TABLE IF NOT EXISTS inverted_index (
        term TEXT NOT NULL,
        doc_id INTEGER NOT NULL,
        term_frequency INTEGER NOT NULL,
        positions TEXT,
        PRIMARY KEY (term, doc_id),
        FOREIGN KEY (doc_id) REFERENCES documents(id)
    );

    CREATE TABLE IF NOT EXISTS tfidf_scores (
        doc_id INTEGER NOT NULL,
        term TEXT NOT NULL,
        tf REAL NOT NULL,
        idf REAL NOT NULL,
        tfidf REAL NOT NULL,
        PRIMARY KEY (doc_id, term),
        FOREIGN KEY (doc_id) REFERENCES documents(id)
    );

    CREATE TABLE IF NOT EXISTS similarity_matrix (
        doc_id_1 INTEGER NOT NULL,
        doc_id_2 INTEGER NOT NULL,
        similarity REAL NOT NULL,
        PRIMARY KEY (doc_id_1, doc_id_2),
        FOREIGN KEY (doc_id_1) REFERENCES documents(id),
        FOREIGN KEY (doc_id_2) REFERENCES documents(id)
    );

    CREATE TABLE IF NOT EXISTS keywords (
        doc_id INTEGER NOT NULL,
        term TEXT NOT NULL,
        score REAL NOT NULL,
        rank INTEGER NOT NULL,
        PRIMARY KEY (doc_id, term),
        FOREIGN KEY (doc_id) REFERENCES documents(id)
    );

    CREATE TABLE IF NOT EXISTS benchmarks (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        num_processes INTEGER NOT NULL,
        num_documents INTEGER NOT NULL,
        total_words INTEGER NOT NULL,
        time_sequential_ms REAL,
        time_parallel_ms REAL,
        speedup REAL,
        efficiency REAL,
        created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
    );

    CREATE TABLE IF NOT EXISTS processing_logs (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        level TEXT NOT NULL,
        source TEXT,
        message TEXT NOT NULL,
        details TEXT,
        created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
    );

    CREATE TABLE IF NOT EXISTS search_history (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        query TEXT NOT NULL,
        num_results INTEGER DEFAULT 0,
        time_ms REAL,
        created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
    );

    CREATE INDEX IF NOT EXISTS idx_word_counts_doc ON word_counts(doc_id);
    CREATE INDEX IF NOT EXISTS idx_word_counts_term ON word_counts(term);
    CREATE INDEX IF NOT EXISTS idx_inverted_index_term ON inverted_index(term);
    CREATE INDEX IF NOT EXISTS idx_tfidf_doc ON tfidf_scores(doc_id);
    CREATE INDEX IF NOT EXISTS idx_tfidf_term ON tfidf_scores(term);
    CREATE INDEX IF NOT EXISTS idx_similarity_doc1 ON similarity_matrix(doc_id_1);
    CREATE INDEX IF NOT EXISTS idx_similarity_doc2 ON similarity_matrix(doc_id_2);
    CREATE INDEX IF NOT EXISTS idx_keywords_doc ON keywords(doc_id);
''')

conn.commit()
conn.close()
print('Database initialized successfully at: ' + db_path)
"@

$initScriptPath = Join-Path (Join-Path $ProjectRoot "data") "_init_db.py"
$initScript | Out-File -FilePath $initScriptPath -Encoding UTF8

& $pythonCmd $initScriptPath
if ($LASTEXITCODE -eq 0) {
    Write-Step "Base de datos SQLite inicializada: data/paralex.db"
} else {
    Write-Err "Error inicializando la base de datos"
}

Remove-Item $initScriptPath -ErrorAction SilentlyContinue

# ==============================================================================
# 7. Build C++ Project (if compiler and cmake available)
# ==============================================================================
if (-not $SkipBuild -and $hasCompiler -and $cmake) {
    Write-Header "7. Compilando proyecto C++"
    
    $buildDir = Join-Path $ProjectRoot "build"
    
    Push-Location $buildDir
    try {
        Write-Step "Ejecutando CMake..."
        & cmake .. 2>&1
        if ($LASTEXITCODE -eq 0) {
            Write-Step "Compilando..."
            & cmake --build . --config Release 2>&1
            if ($LASTEXITCODE -eq 0) {
                Write-Step "Compilacion exitosa"
            } else {
                Write-Warning "Error en la compilacion. Verificar dependencias."
            }
        } else {
            Write-Warning "Error en CMake. Intentando con MinGW..."
            & cmake .. -G "MinGW Makefiles" 2>&1
            if ($LASTEXITCODE -eq 0) {
                & cmake --build . 2>&1
            }
        }
    } finally {
        Pop-Location
    }
} else {
    if ($SkipBuild) {
        Write-Step "Saltando compilacion (--SkipBuild)"
    } else {
        Write-Warning "Saltando compilacion: falta compilador o CMake"
    }
}

# ==============================================================================
# 8. Summary
# ==============================================================================
Write-Header "RESUMEN DE CONFIGURACION"

Write-Host "  Proyecto:      Paralex" -ForegroundColor White
Write-Host "  Ubicacion:     $ProjectRoot" -ForegroundColor White
Write-Host "  Python:        $pythonCmd" -ForegroundColor White
Write-Host "  Compilador:    $(if($hasCompiler){$compilerInfo}else{'No encontrado'})" -ForegroundColor $(if($hasCompiler){'Green'}else{'Yellow'})
Write-Host "  CMake:         $(if($cmake){'Disponible'}else{'No encontrado'})" -ForegroundColor $(if($cmake){'Green'}else{'Yellow'})
Write-Host "  MS-MPI:        $(if($mpiExec){'Disponible'}else{'No encontrado'})" -ForegroundColor $(if($mpiExec){'Green'}else{'Yellow'})
Write-Host "  Base de datos: data/paralex.db" -ForegroundColor White
Write-Host ""

Write-Header "SIGUIENTES PASOS"
if (-not $mpiExec -or -not $msmpiInc) {
    Write-Host "  1. Instalar MS-MPI Runtime + SDK" -ForegroundColor Yellow
    Write-Host "     https://learn.microsoft.com/en-us/message-passing-interface/microsoft-mpi" -ForegroundColor Blue
}
if (-not $hasCompiler) {
    Write-Host "  2. Instalar Visual Studio Build Tools o MinGW" -ForegroundColor Yellow
}
if (-not $cmake) {
    Write-Host "  3. Instalar CMake" -ForegroundColor Yellow
}
Write-Host "  4. Ejecutar: .\setup.ps1  (para recompilar)" -ForegroundColor White
Write-Host "  5. Ejecutar: streamlit run frontend/app.py" -ForegroundColor White
Write-Host ""
