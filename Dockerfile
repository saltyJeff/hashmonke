FROM mcr.microsoft.com/devcontainers/cpp:dev-debian13

RUN apt-get update && export DEBIAN_FRONTEND=noninteractive \
    && apt-get -y install --no-install-recommends \
    cmake \
    ninja-build \
    clang-format \
    clang-tidy \
    build-essential \
    curl \
    unzip \
    ca-certificates \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/*


ARG COSMOCC_URL="https://github.com/jart/cosmopolitan/releases/download/4.0.2/cosmocc-4.0.2.zip"
ARG COSMOCC_SHA1="8703531b99fa3c72241905834a6e5f77203cb137"

RUN mkdir -p /opt/cosmocc /tmp/cosmocc \
    && curl -fsSL "${COSMOCC_URL}" -o /tmp/cosmocc/cosmocc.zip \
    && echo "${COSMOCC_SHA1}  /tmp/cosmocc/cosmocc.zip" | sha1sum -c - \
    && unzip -q /tmp/cosmocc/cosmocc.zip -d /opt/cosmocc \
    && chmod +x /opt/cosmocc/bin/* \
    && rm -rf /tmp/cosmocc

ENV PATH="/opt/cosmocc/bin:${PATH}"