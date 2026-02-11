FROM mcr.microsoft.com/dotnet/sdk:10.0

RUN apt-get update \
    && apt-get install -y --no-install-recommends \
        build-essential \
        cmake \
        ninja-build \
        pkg-config \
        libffi-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /workspace

CMD ["bash"]
