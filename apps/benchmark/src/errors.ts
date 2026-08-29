export class InfrastructureError extends Error {
  cause: unknown;

  constructor(message: string, cause?: unknown) {
    super(message);
    this.name = 'InfrastructureError';
    this.cause = cause;
  }
}

export class ProductError extends Error {
  cause: unknown;

  constructor(message: string, cause?: unknown) {
    super(message);
    this.name = 'ProductError';
    this.cause = cause;
  }
}

export function isProductError(error: any): boolean {
  return error instanceof ProductError || error?.name === 'ProductError';
}

export function isInfrastructureError(error: any): boolean {
  if (error instanceof InfrastructureError || error?.name === 'InfrastructureError') return true;
  if (['EACCES', 'EIO', 'EMFILE', 'ENFILE', 'ENOENT', 'ENOSPC', 'EROFS'].includes(error?.code)) return true;
  return error?.cause ? isInfrastructureError(error.cause) : false;
}
