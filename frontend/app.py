from flask import Flask, render_template, request, jsonify
import subprocess
import tempfile
import os
import re

app = Flask(__name__)

# --------------------------------------------------------
# Paths
# --------------------------------------------------------

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))

CHECKER = os.path.join(
    ROOT,
    "build",
    "tools",
    "flang-mpi-checker",
    "flang-mpi-checker"
)

MODDIR = os.path.join(
    ROOT,
    "build",
    "mpi_mods"
)

TESTS = {

    "buffer_mismatch": [

        "t01_basic_mismatch.f90",
        "t02_kind_mismatch.f90",
        "t03_assumed_shape_buffer.f90",
        "t04_sendrecv_pair.f90"

    ],

    "noncontiguous": [

        "t05_stride2_section.f90",
        "t06_2d_column_section.f90",
        "t07_assumed_shape_no_contiguous.f90",
        "t08_contiguous_attr_ok.f90"

    ],

    "derived_type": [

        "t09_no_bind_c.f90",
        "t10_allocatable_component.f90",
        "t11_pointer_component.f90",
        "t12_correct_bind_c.f90",
        "t13_type_create_struct.f90"

    ],

    "optional_args": [

        "t14_optional_buf_no_present.f90",
        "t15_optional_status_dropped.f90",
        "t16_optional_correct_guard.f90"

    ],

    "collective_ordering": [

        "t17_collective_inside_if.f90",
        "t18_mismatched_branches.f90",
        "t19_correct_ordering.f90"

    ],

    "fortran_specific": [

        "t20_module_wrapper.f90",
        "t21_array_of_structs.f90",
        "t22_coarray_mixed.f90"

    ]

}

# --------------------------------------------------------
# Home
# --------------------------------------------------------

@app.route("/")
def index():
    return render_template("index.html")

# --------------------------------------------------------
# Analyze
# --------------------------------------------------------

@app.route("/analyze", methods=["POST"])
def analyze():

    code = request.form.get("code", "")
    uploaded = request.files.get("file")

    fd, path = tempfile.mkstemp(suffix=".f90")
    os.close(fd)

    try:

        if uploaded and uploaded.filename:
            uploaded.save(path)

        else:
            with open(path, "w") as f:
                f.write(code)

        cmd = [
            CHECKER,
            path,
            "-I",
            MODDIR
        ]

        result = subprocess.run(
            cmd,
            capture_output=True,
            text=True
        )

        output = result.stdout + result.stderr

        diagnostics = []

        lines = output.splitlines()

        i = 0

        while i < len(lines):

            line = lines[i]

            if "error [" in line.lower():

                rule = "Unknown"

                m = re.search(r"\[(.*?)\]", line)

                if m:
                    rule = m.group(1)

                call = ""

                c = re.search(r"'(mpi_[^']+)'", line.lower())

                if c:
                    call = c.group(1)

                                # -----------------------------
                # Clean the diagnostic message
                # -----------------------------

                clean = line

                # Remove "<unknown>:"
                clean = re.sub(r"^<unknown>:\s*", "", clean, flags=re.IGNORECASE)

                # Remove "error [Rule]:"
                clean = re.sub(
                    r"error\s*\[[^\]]+\]\s*:\s*",
                    "",
                    clean,
                    flags=re.IGNORECASE
                )

                # Extract available bytes
                available = ""

                m = re.search(
                    r"buffer has (\d+) byte",
                    clean,
                    flags=re.IGNORECASE
                )

                if m:
                    available = m.group(1)

                # Extract required bytes
                required = ""

                m = re.search(
                    r"requires (\d+) byte",
                    clean,
                    flags=re.IGNORECASE
                )

                if m:
                    required = m.group(1)

                # Extract issue
                issue = ""

                if "overflow" in clean.lower():
                    issue = "Potential Buffer Overflow"

                elif "underflow" in clean.lower():
                    issue = "Potential Buffer Underflow"

                else:
                    issue = "MPI Correctness Violation"

                message = clean
                fix = ""

                if i + 1 < len(lines):

                    nxt = lines[i + 1].strip()

                    if nxt.lower().startswith("fix:"):

                        fix = nxt[4:].strip()

                        i += 1

                diagnostics.append({

                    "rule": rule,

                    "severity": "Error",

                    "mpi_call": call,

                    "message": message,

                    "available": available,

                    "required": required,

                    "issue": issue,

                    "fix": fix

                })

            i += 1

        errors = 0
        warnings = 0

        m = re.search(r"Errors\s*:\s*(\d+)", output)

        if m:
            errors = int(m.group(1))

        m = re.search(r"Warnings\s*:\s*(\d+)", output)

        if m:
            warnings = int(m.group(1))

        return jsonify({

            "success": True,

            "summary": {

                "errors": errors,

                "warnings": warnings,

                "diagnostic_count": len(diagnostics)

            },

            "diagnostics": diagnostics,

            "raw_output": output,

            "return_code": result.returncode

        })

    except Exception as e:

        return jsonify({

            "success": False,

            "error": str(e)

        })

    finally:

        try:
            os.remove(path)
        except:
            pass

# --------------------------------------------------------

@app.route("/tests/<category>")
def tests(category):

    return jsonify(

        TESTS.get(category, [])

    )

@app.route("/demo", methods=["POST"])
def run_demo():

    data = request.get_json()

    category = data["category"]

    test = data["test"]

    filepath = os.path.join(

        ROOT,

        "test",

        category,

        test

    )

    if not os.path.exists(filepath):

        return jsonify({

            "success": False,

            "error": "Test file not found."

        })

    with open(filepath, "r") as f:

        code = f.read()

    return jsonify({

        "success": True,

        "code": code

    })

if __name__ == "__main__":

    app.run(debug=True)