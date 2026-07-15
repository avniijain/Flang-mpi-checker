const categorySelect = document.getElementById("demoCategory");
const testSelect = document.getElementById("demoTest");
const demoBtn = document.getElementById("demoBtn");

const runBtn = document.getElementById("run");

const output = document.getElementById("out");

const errorCount = document.getElementById("errorCount");
const warningCount = document.getElementById("warningCount");

categorySelect.addEventListener("change", async () => {

    testSelect.innerHTML =
        "<option>Loading...</option>";

    const response = await fetch(
        "/tests/" + categorySelect.value
    );

    const tests = await response.json();

    testSelect.innerHTML = "";

    tests.forEach(test => {

        const option =
            document.createElement("option");

        option.value = test;

        option.textContent =
            test.replace(".f90", "");

        testSelect.appendChild(option);

    });

});

runBtn.addEventListener("click", analyzeSource);

async function analyzeSource() {

    const formData = new FormData();

    const file = document.getElementById("file").files[0];

    const code = document.getElementById("code").value.trim();

    if (file)
        formData.append("file", file);

    else
        formData.append("code", code);

    runBtn.disabled = true;

    runBtn.innerHTML = `
        <span class="spinner-border spinner-border-sm"></span>
        &nbsp;Analyzing...
    `;

    output.innerHTML = `
        <div class="terminal-placeholder">
            Parsing source...<br>
            Extracting MPI calls...<br>
            Running correctness rules...
        </div>
    `;

    try {

        const response = await fetch("/analyze", {

            method: "POST",

            body: formData

        });

        const result = await response.json();

        runBtn.disabled = false;

        runBtn.innerHTML = "Analyze Source";

        if (!result.success) {

            output.innerHTML = `

                <div class="diag-card">

                    <div class="diag-rule">

                        ❌ Backend Error

                    </div>

                    <div class="diag-msg">

                        ${result.error}

                    </div>

                </div>

            `;

            return;

        }

        //----------------------------------------------------
        // Summary
        //----------------------------------------------------

        errorCount.textContent = result.summary.errors;

        warningCount.textContent = result.summary.warnings;

        //----------------------------------------------------
        // Diagnostics
        //----------------------------------------------------

        let html = "";

        if (result.diagnostics.length === 0) {

            html += `

                <div class="diag-card" style="border-left-color:#22c55e">

                    <div class="diag-rule" style="color:#22c55e">

                        ✔ No Problems Found

                    </div>

                    <div class="diag-msg">

                        The checker did not report any diagnostics.

                    </div>

                </div>

            `;

        }

        else {

            result.diagnostics.forEach(diag => {

                html += `

                    <div class="diag-card">

                        <div class="diag-rule">

                            🔴 ${diag.rule}

                        </div>

                        <div class="diag-msg">

                            <strong>MPI Call</strong><br>

                            ${diag.mpi_call || "-"}

                        </div>

                        <div class="diag-msg mt-3">

                            <strong>Problem</strong><br>

${diag.issue}

${diag.available ? `

<div class="diag-msg mt-3">

<strong>Available Buffer</strong><br>

${diag.available} bytes

</div>

` : ""}

${diag.required ? `

<div class="diag-msg mt-3">

<strong>Required Buffer</strong><br>

${diag.required} bytes

</div>

` : ""}

                        </div>

                        ${diag.fix ? `

                        <div class="diag-fix">

                            <strong>Suggested Fix</strong>

                            <br><br>

                            ${diag.fix}

                        </div>

                        ` : ""}

                    </div>

                `;

            });

        }

        //----------------------------------------------------
        // Raw Output
        //----------------------------------------------------

        html += `

            <details class="mt-4">

                <summary>

                    Raw Terminal Output

                </summary>

                <div class="raw-output">

${result.raw_output}

                </div>

            </details>

        `;

        output.innerHTML = html;

    }

    catch (err) {

        runBtn.disabled = false;

        runBtn.innerHTML = "Analyze Source";

        output.innerHTML = `

            <div class="diag-card">

                <div class="diag-rule">

                    ❌ Connection Error

                </div>

                <div class="diag-msg">

                    ${err}

                </div>

            </div>

        `;

    }

}

demoBtn.addEventListener("click", async () => {

    const response = await fetch("/demo", {

        method: "POST",

        headers: {

            "Content-Type": "application/json"

        },

        body: JSON.stringify({

            category: categorySelect.value,

            test: testSelect.value

        })

    });

    const result = await response.json();

    if (!result.success) {

        alert(result.error);

        return;

    }

    document.getElementById("code").value = result.code;

    analyzeSource();
});